# Venus1 task_manager DMT probe

Run in a licensed VCS environment (validated with VCS T-2022.06-SP2):

```sh
python3 tests/rtl/run_dmt_length.py --rtl-root /path/to/venus_soc --out /path/to/new-run
```

The output directory must be new. Original RTL sources are read only. The test
checks return publication and ordinary dependency decoding for 1, 44, 64, 65,
172, 336, 6148, 12288 and 65535 bytes, plus 65708 -> 172 field truncation.
It forces isolated FSM inputs. It does not run firmware, whole-DAG or bus-level
SoC qualification. Source hashes and build/run status are retained per run.
