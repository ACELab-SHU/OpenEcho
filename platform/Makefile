.PHONY: doctor test smoke forge-check

doctor:
	./ace-echo doctor

test:
	PYTHONPATH=src python3 -m unittest discover -s tests -v
	python3 -m unittest discover -s workloads/5g_lite/tasks/forge_vector_smoke -p test_smoke.py -v

smoke:
	python3 scripts/smoke.py

forge-check:
	./ace-echo forge examples/forge-oai-ue.example.json --validate-only
	.agents/skills/ace-echo-venus-forge/scripts/validate_manifest.py backend configs/backends/venus2p0-16x128.json
