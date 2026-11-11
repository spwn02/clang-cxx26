// A coroutine echo server and client over the ten networking headers (P4100 series): io, io_buffer, io_stream,
// io_context, io_timer, io_signal, io_file, net, net_tls and io_bridge.
#include <io>
#include <io_bridge>
#include <io_buffer>
#include <io_context>
#include <io_file>
#include <io_signal>
#include <io_stream>
#include <io_timer>
#include <net>
#include <net_tls>

#include <cassert>
#include <chrono>
#include <csignal>
#include <string>

using namespace std::io;

task<> echo_session(any_stream& s) {
  char buf[256];
  for (;;) {
    auto [ec, n] = co_await s.read_some(mutable_buffer(buf, sizeof buf));
    if (ec)
      co_return;
    auto [wec, wn] = co_await write(s, const_buffer(buf, n));
    if (wec)
      co_return;
  }
}

task<> serve(tcp_acceptor& acc, int clients) {
  for (int i = 0; i < clients; ++i) {
    auto [ec, sock] = co_await acc.accept();
    if (ec)
      co_return;
    any_stream s(&sock);
    co_await echo_session(s);
  }
}

task<> client(io_context& ctx, endpoint ep, std::string msg, int& ok) {
  tcp_socket sock(ctx);
  auto [cec] = co_await sock.connect(ep);
  if (cec)
    co_return;
  any_stream s(&sock);
  auto [wec, wn] = co_await write(s, const_buffer(msg.data(), msg.size()));
  std::string got(msg.size(), '\0');
  auto [rec, rn] = co_await read(s, mutable_buffer(got.data(), got.size()));
  ok += (!wec && !rec && got == msg);
  sock.close();
}

task<> pause(io_context& ctx) {
  timer t(ctx, std::chrono::milliseconds(1));
  auto [ec] = co_await t.wait();
  (void)ec;
}

task<> wait_for_interrupt(io_context& ctx) {
  signal_set sigs(ctx);
  if (sigs.add(SIGUSR2))
    co_return;
  sigs.cancel();
  co_return;
}

task<> tls_client(tcp_socket& sock, const tls_context& tc) {
  tls_stream tls(&sock, tc);
  auto [ec] = co_await tls.handshake(tls_role::client);
  (void)ec;
}

task<> log_line(io_context& ctx, std::string path, std::string line) {
  stream_file f(ctx);
  if (f.open(path, file_base::write_only | file_base::create | file_base::truncate))
    co_return;
  auto [ec, n] = co_await write(f, const_buffer(line.data(), line.size()));
  (void)ec;
  (void)n;
}

// io_bridge: a task as a std::execution sender
auto as_sender_of_pause(io_context& ctx) { return as_sender(pause(ctx)); }

int main() {
  io_context ctx;
  tcp_acceptor acc(ctx, endpoint(ipv4_address::loopback(), 0));
  endpoint ep = acc.local_endpoint();
  int ok      = 0;
  auto ex     = ctx.get_executor();
  run_async(ex)(serve(acc, 2));
  for (int i = 0; i < 2; ++i)
    run_async(ex)(client(ctx, ep, "hello " + std::to_string(i), ok));
  run_async(ex)(pause(ctx));
  run_async(ex)(wait_for_interrupt(ctx));
  run_async(ex)(log_line(ctx, "/tmp/networking-echo.log", "done\n"));
  ctx.run();
  assert(ok == 2);
  return 0;
}
