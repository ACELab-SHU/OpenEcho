from copy import deepcopy
import io
import json
from pathlib import Path
import unittest

from ace_echo.axi_capture import AcceptedWriteDecoder, CaptureError, rising_samples


CONTRACT = json.loads((Path(__file__).parents[1] / "configs/observers/venus1p0-l1-axi-v1.json").read_text())


def values(**fields):
    raw = {name: 0 for name in CONTRACT["signals"]}
    for key, value in fields.items():
        name, lo, width = CONTRACT["fields"][key]
        raw[name] |= value << lo
    return {name: f"{value:0{CONTRACT['signals'][name]}b}" for name, value in raw.items()}


def transaction(payload, *, stalls=0, destination=0x20000000, ident=3):
    frames = [values(), values(reset=1, go=1, source=0x10010000,
                              destination=destination, length=len(payload))]
    frames.append(values(reset=1, active=1, awvalid=1, awready=1,
                         awaddr=destination & ~63, awid=ident, awsize=6,
                         awburst=1, awlen=(len(payload) + (destination & 63) + 63)//64 - 1))
    offset = destination & 63
    consumed = 0
    while consumed < len(payload):
        chunk = payload[consumed:consumed+64-offset]
        params = dict(reset=1, active=1, wvalid=1, wready=1,
                      wdata=int.from_bytes(chunk, "little") << (offset*8),
                      wstrb=((1 << len(chunk))-1) << offset,
                      wlast=int(consumed+len(chunk) == len(payload)))
        for _ in range(stalls):
            frames.append(values(**{**params, "wready": 0}))
        frames.append(values(**params))
        consumed += len(chunk)
        offset = 0
    frames.extend([values(reset=1, active=1, bvalid=1, bready=1, bid=ident),
                   values(reset=1, done=1), values(reset=1)])
    return frames


def decode(frames):
    decoder = AcceptedWriteDecoder(CONTRACT)
    for index, sample in enumerate(frames):
        decoder.sample(index * 10, sample)
    return decoder.finish()


def modify(sample, name, value):
    signal, lo, width = CONTRACT["fields"][name]
    bits = sample[signal]
    end = len(bits)-lo
    sample[signal] = bits[:end-width] + (f"{value:0{width}b}" if isinstance(value, int) else value) + bits[end:]


class AcceptedBeatTests(unittest.TestCase):
    def test_no_stall_exact_and_repeated_equal_beats(self):
        data = bytes(range(64))*2
        result = decode(transaction(data))[0]
        self.assertEqual(result["payload"], data)
        self.assertEqual(result["accepted_beats"], 2)
        self.assertEqual(result["responses"], 1)

    def test_backpressure_never_duplicates_beats(self):
        data = bytes(range(128))
        result = decode(transaction(data, stalls=3))[0]
        self.assertEqual(result["payload"], data)
        self.assertEqual(result["accepted_beats"], 2)
        self.assertEqual(result["stalled_w_cycles"], 6)

    def test_awlen_33_and_backend_maximum(self):
        for beats in (34, 64):
            with self.subTest(beats=beats):
                data = bytes(range(64))*beats
                self.assertEqual(decode(transaction(data))[0]["payload"], data)

    def test_partial_strobe_unknown_disabled_bytes(self):
        frames = transaction(b"12345", destination=0x20000003)
        bits = frames[3]["axi_req_o"]
        signal, lo, width = CONTRACT["fields"]["wdata"]
        end = len(bits)-lo
        data = list(bits[end-width:end])
        for lane in range(64):
            if not 3 <= lane < 8:
                data[512-8*(lane+1):512-8*lane] = "x"*8
        modify(frames[3], "wdata", "".join(data))
        self.assertEqual(decode(frames)[0]["payload"], b"12345")

    def test_missing_aw_wlast_b_and_extra_beats_fail(self):
        data = b"a"*128
        for kind in ("aw", "last", "b", "duplicate"):
            with self.subTest(kind=kind):
                frames = transaction(data)
                if kind == "aw":
                    frames.pop(2)
                elif kind == "last":
                    modify(frames[4], "wlast", 0)
                elif kind == "b":
                    frames.pop(5)
                else:
                    frames.insert(4, deepcopy(frames[3]))
                with self.assertRaises(CaptureError):
                    decode(frames)

    def test_unknown_valid_byte_rejects(self):
        frames = transaction(b"a"*64)
        modify(frames[3], "wdata", "x"*512)
        with self.assertRaisesRegex(CaptureError, "unknown accepted"):
            decode(frames)

    def test_data_changed_while_stalled_rejects(self):
        frames = transaction(b"a"*64, stalls=1)
        modify(frames[4], "wdata", 123)
        with self.assertRaisesRegex(CaptureError, "backpressure"):
            decode(frames)

    def test_wrong_valid_byte_and_reordered_payload_fail_exact_comparison(self):
        expected = bytes(range(128))
        for data in (bytes([255])+expected[1:], expected[64:]+expected[:64]):
            with self.subTest(data=data[:4]):
                # Transport can be legal while data is wrong; the independent
                # golden comparison must catch it, regardless of GPIO/CRC.
                actual = decode(transaction(data))[0]["payload"]
                self.assertNotEqual(actual, expected)

    def test_wrong_response_id_error_or_early_response_rejects(self):
        for field, val in (("bid", 4), ("bresp", 2), ("bresp", 3)):
            frames = transaction(b"a"*64)
            modify(frames[4], field, val)
            with self.assertRaises(CaptureError):
                decode(frames)
        frames = transaction(b"a"*64)
        for name in ("bvalid", "bready"):
            modify(frames[3], name, 1)
        modify(frames[3], "bid", 3)
        with self.assertRaisesRegex(CaptureError, "precedes"):
            decode(frames)

    def test_burst_boundary_length_size_and_holes_reject(self):
        for field, value in (("awaddr", 0x20000fc0), ("awlen", 64), ("awsize", 5), ("awburst", 0)):
            frames = transaction(b"a"*128)
            modify(frames[2], field, value)
            with self.assertRaises(CaptureError):
                decode(frames)
        frames = transaction(b"a"*128)
        modify(frames[3], "wstrb", (1 << 64)-2)
        with self.assertRaises(CaptureError):
            decode(frames)

    def test_empty_reset_unknown_and_truncation_reject(self):
        with self.assertRaises(CaptureError):
            decode([])
        frames = transaction(b"a"*64)
        for mutated in (frames[1:], frames[:-3], frames[:4]+[values()]):
            with self.assertRaises(CaptureError):
                decode(mutated)
        modify(frames[2], "awready", "x")
        with self.assertRaises(CaptureError):
            decode(frames)

    def test_multiple_transfers_are_not_merged(self):
        frames = transaction(b"a"*64) + transaction(b"b"*64)[1:]
        self.assertEqual([t["payload"] for t in decode(frames)], [b"a"*64, b"b"*64])

    def test_complete_vcd_pipeline_with_actual_packed_field_layout(self):
        payload = bytes(range(128))
        codes = {name: chr(65+i) for i, name in enumerate(CONTRACT["signals"])}
        header = "$timescale 1ps $end\n"
        for scope in CONTRACT["hierarchy"].split("."):
            header += f"$scope module {scope} $end\n"
        for name, width in CONTRACT["signals"].items():
            header += f"$var wire {width} {codes[name]} {name} $end\n"
        header += "$upscope $end\n"*len(CONTRACT["hierarchy"].split("."))
        header += "$enddefinitions $end\n"
        body = ""
        frames = transaction(payload, stalls=2, ident=0x345)
        for i, frame in enumerate(frames):
            body += f"#{10*i}\n"
            for name, value in frame.items():
                body += f"b{value} {codes[name]}\n"
            body += f"#{10*i+5}\n1{codes['clk']}\n"
        decoder = AcceptedWriteDecoder(CONTRACT)
        for time, sample in rising_samples(io.StringIO(header+body), CONTRACT):
            decoder.sample(time, sample)
        self.assertEqual(decoder.finish()[0]["payload"], payload)

    def test_outstanding_bursts_and_different_id_responses(self):
        frames = transaction(b"a"*128)
        modify(frames[1], "length", 256)
        frames.insert(3, values(reset=1, active=1, awvalid=1, awready=1,
                                awaddr=0x20000080, awid=4, awsize=6, awburst=1, awlen=1))
        frames[6:6] = [deepcopy(frames[4]), deepcopy(frames[5])]
        frames.insert(8, values(reset=1, active=1, bvalid=1, bready=1, bid=4))
        result = decode(frames)[0]
        self.assertEqual(result["payload"], b"a"*256)
        self.assertEqual(result["bursts"], 2)
        self.assertEqual(result["responses"], 2)


class VcdBoundaryTests(unittest.TestCase):
    contract = {"signals": {"clk": 1, "data": 4}, "clock": "clk",
                "hierarchy": "tb.dma", "timescale": "1ps"}
    header = ("$timescale\n1 ps\n$end\n$scope module tb $end\n$scope module dma $end\n"
              "$var wire 1 ! clk $end\n$var reg 4 @ data [3:0] $end\n"
              "$upscope $end\n$upscope $end\n$enddefinitions $end\n")

    def samples(self, text, header=None):
        return list(rising_samples(io.StringIO((header or self.header)+text), self.contract))

    def test_samples_pre_edge_not_post_nba_independent_of_line_order(self):
        for changes in ("1!\nb0010 @\n", "b0010 @\n1!\n"):
            text = "#0\n$dumpvars\n0!\nb1 @\n$end\n#10\n"+changes+"#15\n0!\n#20\n1!\n"
            self.assertEqual(self.samples(text), [(10, {"clk": "0", "data": "0001"}),
                                                   (20, {"clk": "0", "data": "0010"})])

    def test_same_timestamp_delta_blocks_are_one_sampling_instant(self):
        text = "#0\n0!\nb1 @\n#10\n1!\n#10\nb10 @\n#15\n0!\n"
        self.assertEqual(self.samples(text), [(10, {"clk": "0", "data": "0001"})])

    def test_clock_glitch_gap_reversed_time_and_width_fail(self):
        for body in ("#0\n0!\nb0 @\n#10\n1!\n0!\n",
                     "#0\n0!\nb0 @\n#10\nx!\n",
                     "#0\n0!\nb0 @\n$dumpoff\n",
                     "#10\n0!\nb0 @\n#0\n", "#0\n0!\nb00000 @\n"):
            with self.assertRaises(CaptureError):
                self.samples(body)
        with self.assertRaises(CaptureError):
            self.samples("", self.header.replace("wire 1", "wire 2"))

    def test_missing_duplicate_signals_and_timescale_fail(self):
        for header in (self.header.replace("$var wire 1 ! clk $end\n", ""),
                       self.header.replace("$var wire 1 ! clk $end", "$var wire 1 ! clk $end\n$var wire 1 # clk $end"),
                       self.header.replace("1 ps", "10 ps")):
            with self.assertRaises(CaptureError):
                self.samples("", header)


if __name__ == "__main__":
    unittest.main()
