#!/bin/bash

# --- 正确的循环 ---
for file in tests/test/samples_lex_and_syntax/*.cact
do
	./build/cactc $file
done