# POPS/PSXCore timing observations

Observe guest clocks and interrupt delivery at nominal settings through existing interfaces. Never troubleshoot or modify PCSX2. Custom-host profiling patches and runners have been removed. No timing repair has been established.

`code_probe.py` reads 4â€“512 aligned live code bytes through PINE and requires a unique exact match in a privately supplied reference file. Its output contains hashes and the offset, without code bytes. Identity does not establish function semantics.

```text
python code_probe.py --port PORT --address ADDRESS --bytes 64 --reference private/reference.bin --output output/identity.json
```

`counter_difference.py` reconstructs a wrapping clock as a deadline minus a remaining budget. Validate that formula against the exact implementation before using the result. Backwards discontinuities produce no rate.

```text
python counter_difference.py --input output/counters.json --left DEADLINE_FIELD --right REMAINING_FIELD --output output/clock.json
```

Measurements from earlier modified hosts do not establish stock-host or physical-PS2 timing accuracy. New diagnostics belong in POPS/PSXCore.
