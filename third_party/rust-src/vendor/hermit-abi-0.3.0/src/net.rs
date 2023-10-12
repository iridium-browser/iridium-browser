#![allow(nonstandard_style)]

pub const AF_INET: i32 = 0;
pub const AF_INET6: i32 = 1;
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
pub const SHUT_RD: i32 = 0;
pub const SHUT_RDWR: i32 = 2;
pub const SHUT_WR: i32 = 1;
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
pub const FIONBIO: i32 = 0x8008667eu32 as i32;
pub const EAI_NONAME: i32 = -2200;
pub const EAI_SERVICE: i32 = -2201;
pub const EAI_FAIL: i32 = -2202;
pub const EAI_MEMORY: i32 = -2203;
pub const EAI_FAMILY: i32 = -2204;
pub type sa_family_t = u8;
pub type socklen_t = u32;
pub type in_addr_t = u32;
pub type in_port_t = u16;
pub type nfds_t = usize;

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct in_addr {
	pub s_addr: u32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct in6_addr {
	pub s6_addr: [u8; 16],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr {
    pub sa_len: u8,
    pub sa_family: sa_family_t,
    pub sa_data: [u8; 14],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_in {
    pub sin_len: u8,
    pub sin_family: sa_family_t,
    pub sin_port: in_port_t,
    pub sin_addr: in_addr,
    pub sin_zero: [u8; 8],
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct sockaddr_in6 {
	pub sin6_family: sa_family_t,
	pub sin6_port: in_port_t,
	pub sin6_addr: in6_addr,
	pub sin6_flowinfo: u32,
	pub sin6_scope_id: u32,
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
    pub ipv6mr_interface: u32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct addrinfo {
    pub ai_flags: i32,
    pub ai_family: i32,
    pub ai_socktype: i32,
    pub ai_protocol: i32,
    pub ai_addrlen: socklen_t,
    pub ai_addr: *mut sockaddr,
    pub ai_canonname: *mut u8,
    pub ai_next: *mut addrinfo,
}

extern "C" {
    #[link_name = "sys_accept"]
    pub fn accept(s: i32, addr: *mut sockaddr, addrlen: *mut socklen_t) -> i32;

    #[link_name = "sys_bind"]
    pub fn bind(s: i32, name: *const sockaddr, namelen: socklen_t) -> i32;

    #[link_name = "sys_connect"]
    pub fn connect(s: i32, name: *const sockaddr, namelen: socklen_t) -> i32;

    #[link_name = "sys_close"]
    pub fn close(s: i32) -> i32;

    #[link_name = "sys_dup"]
    pub fn dup(s: i32) -> i32;

    #[link_name = "sys:getpeername"]
    pub fn getpeername(s: i32, name: *mut sockaddr, namelen: *mut socklen_t) -> i32;

    #[link_name = "sys_getsockname"]
    pub fn getsockname(s: i32, name: *mut sockaddr, namelen: *mut socklen_t) -> i32;

    #[link_name = "sys_getsockopt"]
    pub fn getsockopt(
        s: i32,
        level: i32,
        optname: i32,
        optval: *mut c_void,
        optlen: *mut socklen_t,
    ) -> i32;

    #[link_name = "sys_setsockopt"]
    pub fn setsockopt(
        s: i32,
        level: i32,
        optname: i32,
        optval: *const c_void,
        optlen: socklen_t,
    ) -> i32;

    #[link_name = "sys_ioctl"]
    pub fn ioctl(s: i32, cmd: c_long, argp: *mut c_void) -> i32;

    #[link_name = "sys_listen"]
    pub fn listen(s: i32, backlog: c_int) -> i32;

    #[link_name = "sys_poll"]
    pub fn pollfd(fds: *mut pollfd, nfds: nfds_t, timeout: i32);

    #[link_name = "sys_recv"]
    pub fn recv(s: c_int, mem: *mut c_void, len: size_t, flags: c_int) -> isize;

    #[link_name = "sys_read"]
    pub fn read(s: c_int, mem: *mut c_void, len: size_t) -> isize;

    #[link_name = "sys_readv"]
    pub fn readv(s: c_int, bufs: *const iovec, bufcnt: c_int) -> isize;

    #[link_name = "sys_recvfrom"]
    pub fn recvfrom(
        s: c_int,
        mem: *mut c_void,
        len: size_t,
        flags: c_int,
        from: *mut sockaddr,
        fromlen: *mut socklen_t,
    ) -> isize;

    #[link_name = "sys_send"]
    pub fn send(s: i32, mem: *const c_void, len: usize, flags: i32) -> isize;

    #[link_name = "sys_sendmsg"]
    pub fn sendmsg(s: c_int, message: *const msghdr, flags: c_int) -> isize;

    #[link_name = "sys_sendto"]
    pub fn sendto(
        s: c_int,
        mem: *const c_void,
        len: size_t,
        flags: c_int,
        to: *const sockaddr,
        tolen: socklen_t,
    ) -> ssize_t;

    #[link_name = "sys_shutdown"]
    pub fn shutdown(s: i32, how: i32) -> i32;

    #[link_name = "sys_socket"]
    pub fn socket(domain: i32, type_: i32, protocol: i32) -> i32;

    #[link_name = "sys_write"]
    pub fn write(s: c_int, mem: *const c_void, len: size_t) -> ssize_t;

    #[link_name = "sys_writev"]
    pub fn writev(s: c_int, bufs: *const iovec, bufcnt: c_int) -> ssize_t;

    #[link_name = "sys_freeaddrinfo"]
    pub fn freeaddrinfo(ai: *mut addrinfo);

    #[link_name = "sys_getaddrinfo"]
    pub fn getaddrinfo(
        nodename: *const u8,
        servname: *const u8,
        hints: *const addrinfo,
        res: *mut *mut addrinfo,
    ) -> i32;

        #[link_name = "sys_select"]
    pub fn select(
        maxfdp1: i32,
        readset: *mut fd_set,
        writeset: *mut fd_set,
        exceptset: *mut fd_set,
        timeout: *mut timeval,
    ) -> i32;

    #[link_name = "sys_pool"]
    pub fn poll(
        fds: *mut pollfd,
        nfds: nfds_t,
        timeout: i32
    ) -> i32;
}
