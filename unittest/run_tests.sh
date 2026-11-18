#!/usr/bin/env bash
# Chay toan bo unit test bang Ceedling (gcc + ruby cua MSYS2 UCRT64)
export PATH=/c/msys64/ucrt64/bin:$PATH
cd "$(dirname "$0")"
ceedling "${@:-test:all}"
