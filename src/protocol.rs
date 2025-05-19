use bincode::{Decode, Encode};
use std::net::Ipv4Addr;

#[derive(Decode, Encode)]
pub struct JoinAnnouncement {
    text: [u8; 5],
    ipv4: [u8; 4], // expect them to be big endian
}

impl JoinAnnouncement {
    pub fn valid(&self) -> bool {
        &self.text == b"HELLO"
    }

    pub fn ipv4_addr(&self) -> Ipv4Addr {
        Ipv4Addr::from(u32::from_be_bytes(self.ipv4))
    }
}
