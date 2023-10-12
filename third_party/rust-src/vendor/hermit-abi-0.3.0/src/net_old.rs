#![allow(nonstandard_style)]
use crate::errno::EINVAL;
use crate::FileDescriptor;
use libc::{c_char, c_int, c_uint, c_void, size_t, ssize_t};

extern "C" {
	fn sys_hermit_socket(domain: i32, type_: i32, protocol: i32) -> FileDescriptor;
	fn sys_hermit_accept(
		s: FileDescriptor,
		addr: *mut sockaddr,
		addrlen: *mut socklen_t,
	) -> FileDescriptor;
	fn sys_hermit_bind(s: FileDescriptor, name: *const sockaddr, namelen: socklen_t) -> i32;
	fn sys_hermit_listen(s: FileDescriptor, backlog: i32) -> i32;
	fn sys_hermit_shutdown(s: FileDescriptor, how: i32) -> i32;
	fn sys_hermit_getpeername(
		s: FileDescriptor,
		name: *mut sockaddr,
		namelen: *mut socklen_t,
	) -> i32;
	fn sys_hermit_getsockname(
		s: FileDescriptor,
		name: *mut sockaddr,
		namelen: *mut socklen_t,
	) -> i32;
	fn sys_hermit_getsockopt(
		s: FileDescriptor,
		level: i32,
		optname: i32,
		optval: *mut c_void,
		optlen: *const socklen_t,
	) -> i32;
	fn sys_hermit_setsockopt(
		s: FileDescriptor,
		level: i32,
		optname: i32,
		optval: *const c_void,
		optlen: socklen_t,
	) -> i32;
	fn sys_hermit_connect(s: FileDescriptor, name: *const sockaddr, namelen: socklen_t) -> i32;
	fn sys_hermit_getaddrinfo(
		nodename: *const c_char,
		servname: *const i8,
		hints: *const addrinfo,
		res: *mut *mut addrinfo,
	) -> i32;
}

pub const AF_INET: i32 = 10;
pub const AF_INET6: i32 = 2;
pub const IPPROTO_IP: i32 = 0;
pub const IPPROTO_IPV6: i32 = 41;
pub const IPPROTO_TCP: i32 = 6;
pub const IPV6_ADD_MEMBERSHIP: i32 = 12;
pub const IPV6_DROP_MEMBERSHIP: i32 = 13;
pub const IPV6_MULTICAST_LOOP: i32 = 19;
pub const IPV6_V6ONLY: i32 = 27;
pub const IP_TTL: i32 = 2;
pub const IP_MULTICAST_TTL: i32 = 5;
pub const IP_MULTICAST_LOOP: i32 = 7;
pub const IP_ADD_MEMBERSHIP: i32 = 3;
pub const IP_DROP_MEMBERSHIP: i32 = 4;
pub const SHUT_READ: i32 = 0;
pub const SHUT_WRITE: i32 = 1;
pub const SHUT_BOTH: i32 = 2;
pub const SOCK_DGRAM: i32 = 2;
pub const SOCK_STREAM: i32 = 1;
pub const SOL_SOCKET: i32 = 4095;
pub const SO_BROADCAST: i32 = 32;
pub const SO_ERROR: i32 = 4103;
pub const SO_RCVTIMEO: i32 = 4102;
pub const SO_REUSEADDR: i32 = 4;
pub const SO_SNDTIMEO: i32 = 4101;
pub const SO_LINGER: i32 = 128;
pub const TCP_NODELAY: i32 = 1;
pub const MSG_PEEK: i32 = 1;

pub type sa_family_t = u8;
pub type socklen_t = usize;
pub type in_addr_t = u32;
pub type in_port_t = u16;

#[derive(Debug, Copy, Clone)]
#[repr(C)]
pub struct in_addr {
	pub s_addr: u32,
}

#[derive(Debug, Copy, Clone)]
#[repr(C)]
pub struct in6_addr {
	pub s6_addr: [u8; 16],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr {
	pub sa_len: u8,
	pub sa_family: sa_family_t,
	pub sa_data: [u8; 14usize],
}

#[derive(Copy, Clone)]
#[repr(C)]
pub struct sockaddr_in6 {
	pub sin6_len: u8,
	pub sin6_family: sa_family_t,
	pub sin6_port: in_port_t,
	pub sin6_flowinfo: u32,
	pub sin6_addr: in6_addr,
	pub sin6_scope_id: u32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_in {
	pub sin_len: u8,
	pub sin_family: sa_family_t,
	pub sin_port: in_port_t,
	pub sin_addr: in_addr,
	pub sin_zero: [u8; 8usize],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct iovec {
	pub iov_base: *mut c_void,
	pub iov_len: usize,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ip_mreq {
	pub imr_multiaddr: in_addr,
	pub imr_interface: in_addr,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct ipv6_mreq {
	pub ipv6mr_multiaddr: in6_addr,
	pub ipv6mr_interface: c_uint,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct msghdr {
	pub msg_name: *mut c_void,
	pub msg_namelen: socklen_t,
	pub msg_iov: *mut iovec,
	pub msg_iovlen: c_int,
	pub msg_control: *mut c_void,
	pub msg_controllen: socklen_t,
	pub msg_flags: c_int,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_storage {
	pub s2_len: u8,
	pub ss_family: sa_family_t,
	pub s2_data1: [c_char; 2usize],
	pub s2_data2: [u32; 3usize],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct addrinfo {
	pub ai_flags: c_int,
	pub ai_family: c_int,
	pub ai_socktype: c_int,
	pub ai_protocol: c_int,
	pub ai_addrlen: socklen_t,
	pub ai_addr: *mut sockaddr,
	pub ai_canonname: *mut c_char,
	pub ai_next: *mut addrinfo,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct linger {
	pub l_onoff: i32,
	pub l_linger: i32,
}

#[inline]
pub unsafe fn socket(domain: c_int, type_: c_int, protocol: c_int) -> FileDescriptor {
	sys_hermit_socket(domain, type_, protocol)
}

#[inline]
pub unsafe fn accept(
	s: FileDescriptor,
	addr: *mut sockaddr,
	addrlen: *mut socklen_t,
) -> FileDescriptor {
	sys_hermit_accept(s, addr, addrlen)
}

#[inline]
pub unsafe fn bind(s: FileDescriptor, name: *const sockaddr, namelen: socklen_t) -> i32 {
	sys_hermit_bind(s, name, namelen)
}

#[inline]
pub unsafe fn shutdown(s: FileDescriptor, how: c_int) -> i32 {
	sys_hermit_shutdown(s, how)
}

#[inline]
pub unsafe fn getpeername(s: FileDescriptor, name: *mut sockaddr, namelen: *mut socklen_t) -> i32 {
	sys_hermit_getpeername(s, name, namelen)
}

#[inline]
pub unsafe fn getsockname(s: FileDescriptor, name: *mut sockaddr, namelen: *mut socklen_t) -> i32 {
	sys_hermit_getsockname(s, name, namelen)
}

#[inline]
pub unsafe fn getsockopt(
	s: FileDescriptor,
	level: c_int,
	optname: c_int,
	optval: *mut c_void,
	optlen: *const socklen_t,
) -> i32 {
	sys_hermit_getsockopt(s, level, optname, optval, optlen)
}

#[inline]
pub unsafe fn setsockopt(
	s: FileDescriptor,
	level: c_int,
	optname: c_int,
	optval: *const c_void,
	optlen: socklen_t,
) -> i32 {
	sys_hermit_setsockopt(s, level, optname, optval, optlen)
}

#[inline]
pub unsafe fn connect(s: FileDescriptor, name: *const sockaddr, namelen: socklen_t) -> i32 {
	sys_hermit_connect(s, name, namelen)
}

#[inline]
pub unsafe fn listen(s: FileDescriptor, backlog: c_int) -> i32 {
	sys_hermit_listen(s, backlog)
}

#[inline]
pub unsafe fn recv(s: FileDescriptor, mem: *mut c_void, len: size_t, flags: c_int) -> ssize_t {
	(-EINVAL).try_into().unwrap()
}

#[inline]
pub unsafe fn recvfrom(
	s: FileDescriptor,
	mem: *mut c_void,
	len: size_t,
	flags: c_int,
	from: *mut sockaddr,
	fromlen: *mut socklen_t,
) -> ssize_t {
	(-EINVAL).try_into().unwrap()
}

#[inline]
pub unsafe fn send(s: FileDescriptor, mem: *const c_void, len: size_t, flags: c_int) -> ssize_t {
	(-EINVAL).try_into().unwrap()
}

#[inline]
pub unsafe fn sendmsg(s: FileDescriptor, message: *const msghdr, flags: c_int) -> ssize_t {
	(-EINVAL).try_into().unwrap()
}

#[inline]
pub unsafe fn sendto(
	s: FileDescriptor,
	mem: *const c_void,
	len: size_t,
	flags: c_int,
	to: *const sockaddr,
	tolen: socklen_t,
) -> ssize_t {
	(-EINVAL).try_into().unwrap()
}

#[inline]
pub unsafe fn freeaddrinfo(ai: *mut addrinfo) {}

#[inline]
pub unsafe fn getaddrinfo(
	nodename: *const c_char,
	servname: *const c_char,
	hints: *const addrinfo,
	res: *mut *mut addrinfo,
) -> i32 {
	sys_hermit_getaddrinfo(nodename, servname, hints, res)
}
