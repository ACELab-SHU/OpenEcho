"""Physical master IDs have 9 connected bits, not the 12-bit package width."""
import json
from pathlib import Path
import unittest

from ace_echo.axi_capture import AcceptedWriteDecoder, CaptureError
from test_axi_capture import transaction, modify, CONTRACT as AGGREGATE_CONTRACT


PHYSICAL = json.loads((Path(__file__).parents[1] / "configs/observers/venus1p0-l1-axi-v2.json").read_text())


def decode(frames, contract=PHYSICAL):
    decoder = AcceptedWriteDecoder(contract)
    for index, frame in enumerate(frames):
        decoder.sample(index*10, frame)
    return decoder.finish()


class PhysicalIdTests(unittest.TestCase):
    def test_unconnected_aggregate_bits_are_not_physical_id_bits(self):
        frames = transaction(b"a"*64, ident=0x145)
        modify(frames[4], "bid", "xxx"+f"{0x145:09b}")
        self.assertEqual(decode(frames)[0]["payload"], b"a"*64)
        with self.assertRaisesRegex(CaptureError, "unknown bid"):
            decode(frames, AGGREGATE_CONTRACT)

    def test_each_unknown_connected_id_bit_is_rejected(self):
        for bit in range(9):
            with self.subTest(bit=bit):
                frames = transaction(b"a"*64, ident=0)
                physical = list("0"*9)
                physical[bit] = "x"
                modify(frames[4], "bid", "xxx"+"".join(physical))
                with self.assertRaisesRegex(CaptureError, "unknown bid"):
                    decode(frames)

    def test_each_connected_id_mismatch_is_rejected(self):
        for bit in range(9):
            with self.subTest(bit=bit):
                frames = transaction(b"a"*64, ident=0)
                modify(frames[4], "bid", "xxx"+f"{1 << bit:09b}")
                with self.assertRaisesRegex(CaptureError, "B response"):
                    decode(frames)

    def test_physical_maximum_id_and_unknown_data(self):
        frames = transaction(b"a"*64, ident=511)
        modify(frames[4], "bid", "xxx"+"1"*9)
        self.assertEqual(decode(frames)[0]["payload"], b"a"*64)
        modify(frames[3], "wdata", "x"*512)
        with self.assertRaisesRegex(CaptureError, "unknown accepted valid byte"):
            decode(frames)


if __name__ == "__main__":
    unittest.main()
