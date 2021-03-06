#pragma once

#include <asio/ip/tcp.hpp>

#include <tinyfibers/support/buffer.hpp>

#include <wheels/result/result.hpp>

namespace tinyfibers::net {

class Socket {
  friend class Acceptor;

 public:
  static wheels::Result<Socket> ConnectTo(const std::string& host,
                                          uint16_t port);
  static wheels::Result<Socket> ConnectToLocal(uint16_t port);

  // Non-copyable
  Socket(const Socket&) = delete;
  Socket& operator=(const Socket&) = delete;

  // Movable
  Socket(Socket&&) = default;
  Socket& operator=(Socket&&) = default;

  // Will block until
  // * supplied buffer is full or
  // * end of stream reached or
  // * an error occurred
  // Returns number of bytes received
  // Bytes received < buffer size means end of stream
  wheels::Result<size_t> Read(MutableBuffer buffer);

  // Will block until
  // * at least one byte is read or
  // * end of stream reached or
  // * an error occurred
  // Returns number of bytes received or 0 if end of stream reached
  wheels::Result<size_t> ReadSome(MutableBuffer buffer);

  // Will block until
  // * all the bytes in the buffer are sent or
  // * an error occurred
  wheels::Status Write(ConstBuffer buffer);

  // Shutting down the sender side of the socket
  wheels::Status ShutdownWrite();

 private:
  Socket(asio::ip::tcp::socket&& impl);

 private:
  asio::ip::tcp::socket socket_;
};

}  // namespace tinyfibers::net
