"""Original synthetic regressions; no emulator, game or runtime required."""
import hashlib
from pathlib import Path
import socket
import struct
import sys
import threading
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from pine import Client
from observe import observe
from elf_metadata import inspect
from code_probe import validate
from counter_difference import difference
from state_patch import replace_word

class StatePatchTests(unittest.TestCase):
    def test_only_expected_word_changes(self):
        original=struct.pack('<3I',7,8,9)
        self.assertEqual(replace_word(original,4,8,10),struct.pack('<3I',7,10,9))
        self.assertEqual(struct.unpack_from('<I',original,4)[0],8)
    def test_mismatch_alignment_and_bounds_rejected(self):
        for address,expected in ((4,0),(1,7),(12,7)):
            with self.assertRaises(ValueError):replace_word(struct.pack('<3I',7,8,9),address,expected,10)

class SignatureTests(unittest.TestCase):
    def test_unique_identity_without_exporting_payload(self):
        report=validate(b'ABCD',b'prefixABCDsuffix')
        self.assertEqual(report['reference_offset'],6)
        self.assertTrue(report['unique_exact_match'])
        self.assertNotIn('code',report)

    def test_absent_and_ambiguous_signatures_rejected(self):
        with self.assertRaisesRegex(ValueError,'does not match'):validate(b'ABCD',b'EFGH')
        with self.assertRaisesRegex(ValueError,'ambiguous'):validate(b'ABCD',b'ABCDABCD')

class ProtocolTests(unittest.TestCase):
    def test_fragmented_reply(self):
        client,server=socket.socketpair()
        def answer():
            request=server.recv(1024)
            self.assertEqual(request,struct.pack('<IBI',9,2,0x1000))
            for byte in struct.pack('<IBI',9,0,0x12345678):server.sendall(bytes([byte]))
            server.close()
        thread=threading.Thread(target=answer);thread.start()
        instance=Client.__new__(Client);instance.socket=client
        with instance:self.assertEqual(instance.read32([0x1000]),(0x12345678,))
        thread.join()

    def test_rejected_request_is_not_decoded_as_memory(self):
        client,server=socket.socketpair()
        server.sendall(struct.pack('<IB',5,1))
        instance=Client.__new__(Client);instance.socket=client
        with instance:
            with self.assertRaisesRegex(RuntimeError,'rejected'):instance.read32([0x1000])
        server.close()

    def test_alignment_rejected_before_network(self):
        instance=Client.__new__(Client)
        with self.assertRaises(ValueError):instance.read32([0x1001])

class CounterTests(unittest.TestCase):
    def test_difference_clock_wrap_and_backwards_discontinuity(self):
        report={'summary':[{'name':'deadline'},{'name':'remaining'}],
                'samples':[{'seconds':0,'values':[10,12]}, {'seconds':1,'values':[20,18]}]}
        self.assertEqual(difference(report,'deadline','remaining')['increments_per_second'],4)
        report['samples'][1]['values']=[10,13]
        self.assertIsNone(difference(report,'deadline','remaining')['increments_per_second'])

    def test_wrap_and_state_changes_have_different_semantics(self):
        class Fake:
            def __init__(self):self.index=0
            def read32(self,addresses):
                data=[(0xfffffffe,256),(1,0),(3,256)][self.index];self.index+=1;return data
        clock=iter([0,0,.01,.01,.02,.02,.03,.2])
        fields=[{'name':'count','address':'0x1000','kind':'counter'},
                {'name':'buffer','address':'0x1004','kind':'state'}]
        with patch('observe.time.perf_counter',side_effect=lambda:next(clock)),patch('observe.time.sleep'):
            report=observe(Fake(),fields,.1,.01)
        self.assertAlmostEqual(report['summary'][0]['increments_per_second'],250)
        self.assertNotIn('increments_per_second',report['summary'][1])

class ElfTests(unittest.TestCase):
    def fixture(self):
        data=bytearray(88);data[:7]=b'\x7fELF\x01\x01\x01'
        struct.pack_into('<H',data,18,8);struct.pack_into('<I',data,28,52)
        struct.pack_into('<HH',data,42,32,1)
        struct.pack_into('<8I',data,52,1,84,0x2000,0x2000,4,4,5,4)
        struct.pack_into('<I',data,84,(0x23<<26)|(1<<21)|(2<<16)|16)
        return bytes(data)

    def test_wrong_runtime_fingerprint_is_rejected(self):
        with self.assertRaisesRegex(ValueError,'fingerprint'):inspect(self.fixture(),'0'*64,0x2000,0x2004)

    def test_out_of_file_segment_rejected(self):
        data=bytearray(self.fixture());struct.pack_into('<I',data,56,1000)
        with self.assertRaisesRegex(ValueError,'segment'):inspect(data,hashlib.sha256(data).hexdigest(),0x2000,0x2004)

    def test_metadata_exports_offsets_without_instruction_bytes(self):
        data=self.fixture();report=inspect(data,hashlib.sha256(data).hexdigest(),0x2000,0x2004)
        self.assertEqual(report['events'][0]['offset'],16)
        self.assertNotIn('word',report['events'][0])

if __name__=='__main__':unittest.main()
