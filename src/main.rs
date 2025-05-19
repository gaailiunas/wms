use std::net::{Ipv4Addr, SocketAddrV4};
use std::str::FromStr;

use tokio::net::{TcpListener, UdpSocket};

mod protocol;

const IP: &str = "127.0.0.1";
const PORT: u16 = 8080;

const MULTICAST_IP: &str = "239.255.0.1";
const MULTICAST_PORT: u16 = 12345;

// all integers are big endian
const BINCONFIG: bincode::config::Configuration<bincode::config::BigEndian> =
    bincode::config::standard().with_big_endian();

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = TcpListener::bind(SocketAddrV4::new(Ipv4Addr::from_str(IP)?, PORT)).await?;

    let multicast_sock =
        UdpSocket::bind(SocketAddrV4::new(Ipv4Addr::UNSPECIFIED, MULTICAST_PORT)).await?;
    multicast_sock.set_multicast_loop_v4(true)?;
    multicast_sock.join_multicast_v4(Ipv4Addr::from_str(MULTICAST_IP)?, Ipv4Addr::UNSPECIFIED)?;

    let mut buf = [0; 1024];

    loop {
        tokio::select! {
            Ok((socket, _)) = listener.accept() => {
                tokio::spawn(async move {
                    // do something
                });
            }

            Ok((len, addr)) = multicast_sock.recv_from(&mut buf) => {
                println!("udp multicast: {:?} bytes received from {:?}", len, addr);

                tokio::spawn(async move {
                    if let Ok((decoded, _)) = bincode::decode_from_slice::<protocol::JoinAnnouncement, _>(&buf[..len], BINCONFIG) {
                        if decoded.valid() {
                            println!("udp multicast: announced ipv4 - {:?}", decoded.ipv4_addr());
                        }
                    } else {
                        eprintln!("Failed to decode JoinAnnouncement");
                    }
                });
            }
        }
    }
}
