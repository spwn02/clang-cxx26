//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef TEST_STD_NET_TLS_FIXTURES_H
#define TEST_STD_NET_TLS_FIXTURES_H

// Self-signed test certificates, valid for 100 years, generated with
//   openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:prime256v1 -nodes -days 36500 -subj /CN=localhost \
//     -addext subjectAltName=DNS:localhost,IP:127.0.0.1
// (the second one has CN=other and no subjectAltName). They protect nothing.
namespace tls_fixtures {
inline constexpr const char localhost_cert[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIBmzCCAUGgAwIBAgIUcepbFSHZJ14SI5+5wktAAXwxNAowCgYIKoZIzj0EAwIw
FDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTI2MTExMDIzMTYwMVoYDzIxMjYxMDE3
MjMxNjAxWjAUMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjO
PQMBBwNCAAT6IYNvB7eo5IqjzIQmMw+FHgx/Pwkduupy83crV+SOsnj4uWgP/iD+
sF+Wvf17uftDaLFFzBFKmJOS31mmKOsko28wbTAdBgNVHQ4EFgQU3Iy/B4QtfU2H
OECNmV9A+MQ4qo0wHwYDVR0jBBgwFoAU3Iy/B4QtfU2HOECNmV9A+MQ4qo0wDwYD
VR0TAQH/BAUwAwEB/zAaBgNVHREEEzARgglsb2NhbGhvc3SHBH8AAAEwCgYIKoZI
zj0EAwIDSAAwRQIgLkpiGRuSb2bNqIZTRaIoTswXXsXztsEjeWuqLy4+h58CIQDX
4Wte++HIEz6DDb5PYRkM3EU3YWHpttxuAs3OTHJVvQ==
-----END CERTIFICATE-----
)PEM";

inline constexpr const char localhost_key[] = R"PEM(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgFiHPaCcrk1UR9zwQ
UU+TQZt/BfdKku8XtL3eXUoGTnOhRANCAAT6IYNvB7eo5IqjzIQmMw+FHgx/Pwkd
uupy83crV+SOsnj4uWgP/iD+sF+Wvf17uftDaLFFzBFKmJOS31mmKOsk
-----END PRIVATE KEY-----
)PEM";

inline constexpr const char other_cert[] = R"PEM(-----BEGIN CERTIFICATE-----
MIIBeDCCAR2gAwIBAgIUSZFf0YD/d/74c9QeKGAWm9o3/WEwCgYIKoZIzj0EAwIw
EDEOMAwGA1UEAwwFb3RoZXIwIBcNMjYxMTEwMjMxNjAxWhgPMjEyNjEwMTcyMzE2
MDFaMBAxDjAMBgNVBAMMBW90aGVyMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE
nxT4NrhpsohX+AJ8CbeHtsmaG8VCHGqFrh2uBK/bLQLCLyqdPYb2meIIuV2E9L/M
cVFFXjnKkegZMcfFEDefZKNTMFEwHQYDVR0OBBYEFOqMxNPeBjZRET/EU6Z2586H
srUvMB8GA1UdIwQYMBaAFOqMxNPeBjZRET/EU6Z2586HsrUvMA8GA1UdEwEB/wQF
MAMBAf8wCgYIKoZIzj0EAwIDSQAwRgIhAIYnmshCO+QJa4MnKkH8CIbLfigL5H/4
z7x66dX3/BaIAiEAwux516lcchOcb9yaDdAs63nHK3owBn+SCln1AJbaKFY=
-----END CERTIFICATE-----
)PEM";

inline constexpr const char other_key[] = R"PEM(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgCf49AWfCNLFRlbRM
3zPoncq/kvjPF9ktWq+vLdB37S6hRANCAASfFPg2uGmyiFf4AnwJt4e2yZobxUIc
aoWuHa4Er9stAsIvKp09hvaZ4gi5XYT0v8xxUUVeOcqR6Bkxx8UQN59k
-----END PRIVATE KEY-----
)PEM";

} // namespace tls_fixtures

#endif // TEST_STD_NET_TLS_FIXTURES_H
