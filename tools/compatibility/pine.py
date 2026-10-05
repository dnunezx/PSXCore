"""Small loopback-only PCSX2 PINE client for compatibility diagnostics."""
import socket
import struct

class Client:
    def __init__(self, port, timeout=5):
        self.socket = socket.create_connection(('127.0.0.1', port), timeout)
        self.socket.settimeout(timeout)

    def __enter__(self): return self
    def __exit__(self, *args): self.socket.close()

    def _receive(self, size):
        data = bytearray()
        while len(data) < size:
            block = self.socket.recv(size-len(data))
            if not block: raise RuntimeError('PINE connection closed')
            data.extend(block)
        return bytes(data)

    def request(self, payload, response_bytes):
        self.socket.sendall(struct.pack('<I', len(payload)+4)+payload)
        length = struct.unpack('<I', self._receive(4))[0]
        if not 5 <= length <= 65536: raise RuntimeError('Invalid PINE response length')
        response = self._receive(length-4)
        if response[0] != 0: raise RuntimeError('PINE rejected the request')
        if len(response)-1 != response_bytes: raise RuntimeError('Unexpected PINE response size')
        return response[1:]

    def read32(self, addresses):
        if not addresses or len(addresses)>128: raise ValueError('Require 1–128 addresses')
        if any(not 0<=a<=0xffffffff or a%4 for a in addresses): raise ValueError('Require aligned 32-bit addresses')
        payload=b''.join(struct.pack('<BI',2,a) for a in addresses)
        return struct.unpack('<'+'I'*len(addresses), self.request(payload, 4*len(addresses)))

    def save(self, slot):
        if not 0<=slot<=255: raise ValueError('Slot must be 0–255')
        self.request(struct.pack('<BB',9,slot),0)
