// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

export namespace std {
#if _LIBCPP_HAS_IO_SOCKETS
  // Stage Two of the networking series (platform I/O), experimental: C++29 with -fexperimental-library
  namespace io {
    using std::io::bind_option;
    using std::io::connect;
    using std::io::connect_pair;
    using std::io::host_name;
    using std::io::endpoint;
    using std::io::family;
    using std::io::ip_address;
    using std::io::ipv4_address;
    using std::io::ipv6_address;
    using std::io::local_datagram_socket;
    using std::io::local_endpoint;
    using std::io::local_stream_acceptor;
    using std::io::local_stream_socket;
    using std::io::make_endpoint;
    using std::io::make_ip_address;
    using std::io::make_ipv4_address;
    using std::io::make_ipv6_address;
    using std::io::make_local_endpoint;
    using std::io::message_flags;
    using std::io::operator|;
    using std::io::operator&;
    using std::io::operator~;
    using std::io::operator|=;
    using std::io::operator&=;
    using std::io::resolve_flags;
    using std::io::resolver;
    using std::io::resolver_category;
    using std::io::resolver_entry;
    using std::io::resolver_error;
    using std::io::resolver_results;
    using std::io::reverse_flags;
    using std::io::reverse_resolver_result;
    using std::io::shutdown_type;
    using std::io::tcp_acceptor;
    using std::io::tcp_socket;
    using std::io::udp_socket;

    using std::io::shutdown_both;
    using std::io::shutdown_receive;
    using std::io::shutdown_send;

    namespace socket_option {
      using std::io::socket_option::broadcast;
      using std::io::socket_option::join_group;
      using std::io::socket_option::keep_alive;
      using std::io::socket_option::leave_group;
      using std::io::socket_option::linger;
      using std::io::socket_option::multicast_hops;
      using std::io::socket_option::multicast_loop;
      using std::io::socket_option::no_delay;
      using std::io::socket_option::receive_buffer_size;
      using std::io::socket_option::reuse_address;
      using std::io::socket_option::reuse_port;
      using std::io::socket_option::send_buffer_size;
      using std::io::socket_option::v6_only;
    } // namespace socket_option
  } // namespace io
#endif
} // namespace std
