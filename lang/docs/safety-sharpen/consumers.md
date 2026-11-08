# Phase 2 consumer inventory

Generated from the in-tree `chemical.mod` corpus. The Phase 2 API migration is
source-breaking; every in-tree consumer below is either updated in the same
commit or is a scratch/probe module kept only for regression archaeology.

## Remote imports

No `chemical.mod` in the tree declares a remote/git dependency.

## Compiled applications/examples

These are the first migration targets. The `lang/compiled/examples/` set is the
public example surface; the remaining directories are probes and internal apps.

- `lang/compiled/aes_test`
- `lang/compiled/arc_test`
- `lang/compiled/argc`
- `lang/compiled/armtest`
- `lang/compiled/assignfy`
- `lang/compiled/assoc`
- `lang/compiled/asus`
- `lang/compiled/async_exec`
- `lang/compiled/async_final`
- `lang/compiled/async_generic_poll`
- `lang/compiled/async_mat`
- `lang/compiled/async_probe`
- `lang/compiled/async_ready`
- `lang/compiled/async_smoke`
- `lang/compiled/async_vs`
- `lang/compiled/async_vs2`
- `lang/compiled/atomic_emit_check`
- `lang/compiled/atomic_llvm_check`
- `lang/compiled/atomic_minimal`
- `lang/compiled/atomic_sample`
- `lang/compiled/atomic_tcc_check`
- `lang/compiled/atomic_test`
- `lang/compiled/audit_async`
- `lang/compiled/audit_channel`
- `lang/compiled/audit_dyn`
- `lang/compiled/audit_report`
- `lang/compiled/audit_traits`
- `lang/compiled/audit_vec`
- `lang/compiled/auto_test`
- `lang/compiled/b1_probe`
- `lang/compiled/b24_probe`
- `lang/compiled/b2_probe`
- `lang/compiled/b6_probe`
- `lang/compiled/bcryptbench`
- `lang/compiled/benchprobe`
- `lang/compiled/bignum_diag`
- `lang/compiled/bn_check`
- `lang/compiled/bn_perf`
- `lang/compiled/borrowtest`
- `lang/compiled/box_probe`
- `lang/compiled/cabi_test`
- `lang/compiled/cap`
- `lang/compiled/captest`
- `lang/compiled/cb_positive`
- `lang/compiled/cb_probe`
- `lang/compiled/cbprobe`
- `lang/compiled/chatest`
- `lang/compiled/components`
- `lang/compiled/constread`
- `lang/compiled/ct_smoke`
- `lang/compiled/debug_libs`
- `lang/compiled/decltest`
- `lang/compiled/defcheck`
- `lang/compiled/dyn_probe`
- `lang/compiled/dyn_send_probe`
- `lang/compiled/es`
- `lang/compiled/function_once`
- `lang/compiled/gcm_reference`
- `lang/compiled/generic_loop_bug`
- `lang/compiled/genref`
- `lang/compiled/glob_probe`
- `lang/compiled/hrtb_array_probe`
- `lang/compiled/hrtb_nested`
- `lang/compiled/httprobe`
- `lang/compiled/join_test`
- `lang/compiled/jsonbug`
- `lang/compiled/jsontest`
- `lang/compiled/k3_debug`
- `lang/compiled/large_array_probe`
- `lang/compiled/learn-chemical`
- `lang/compiled/life_probe`
- `lang/compiled/llvm5`
- `lang/compiled/llvm_deadcode`
- `lang/compiled/llvm_smoke`
- `lang/compiled/m65_test`
- `lang/compiled/mbedtls`
- `lang/compiled/md5_test`
- `lang/compiled/min1`
- `lang/compiled/mini_probe`
- `lang/compiled/minimal`
- `lang/compiled/mir_layout_probe`
- `lang/compiled/mir_llvm_probe`
- `lang/compiled/mirasync`
- `lang/compiled/mirdel`
- `lang/compiled/mirllvm_probe`
- `lang/compiled/mirp2`
- `lang/compiled/mirp3`
- `lang/compiled/mirp4`
- `lang/compiled/mirp5`
- `lang/compiled/mirp6`
- `lang/compiled/mongodb`
- `lang/compiled/mpi_test`
- `lang/compiled/mud`
- `lang/compiled/mylib`
- `lang/compiled/neg_for`
- `lang/compiled/neg_guard`
- `lang/compiled/neg_json`
- `lang/compiled/nested3`
- `lang/compiled/noimport_test`
- `lang/compiled/nstest`
- `lang/compiled/p26_scope_spike`
- `lang/compiled/packed_ref`
- `lang/compiled/panictest`
- `lang/compiled/partialtest`
- `lang/compiled/path_test`
- `lang/compiled/perfprobe`
- `lang/compiled/playground`
- `lang/compiled/plugin_repro`
- `lang/compiled/prim_probe`
- `lang/compiled/probe`
- `lang/compiled/probe1`
- `lang/compiled/probe_remote`
- `lang/compiled/quiz`
- `lang/compiled/refget`
- `lang/compiled/repro`
- `lang/compiled/repro_cache`
- `lang/compiled/repro_temp_member`
- `lang/compiled/rsa_crt`
- `lang/compiled/rsa_test`
- `lang/compiled/scoped_dbg`
- `lang/compiled/secure_net`
- `lang/compiled/server_test`
- `lang/compiled/sha256_test`
- `lang/compiled/spantest`
- `lang/compiled/ssr_probe`
- `lang/compiled/stdcall_cb_probe`
- `lang/compiled/stdcall_probe`
- `lang/compiled/strarr`
- `lang/compiled/strconv`
- `lang/compiled/strrepro`
- `lang/compiled/stub_probe`
- `lang/compiled/styletest`
- `lang/compiled/system_test`
- `lang/compiled/talias`
- `lang/compiled/target_probe`
- `lang/compiled/temp`
- `lang/compiled/temp_abi`
- `lang/compiled/temp_assoc`
- `lang/compiled/temp_async`
- `lang/compiled/temp_async_lambda`
- `lang/compiled/temp_atomic`
- `lang/compiled/temp_auto`
- `lang/compiled/temp_bound`
- `lang/compiled/temp_cap`
- `lang/compiled/temp_dyn`
- `lang/compiled/temp_fh_gap`
- `lang/compiled/temp_field_move`
- `lang/compiled/temp_future`
- `lang/compiled/temp_future_gap`
- `lang/compiled/temp_mod`
- `lang/compiled/temp_neg`
- `lang/compiled/temp_safety`
- `lang/compiled/temp_safety_arr`
- `lang/compiled/temp_safety_neg`
- `lang/compiled/temp_send`
- `lang/compiled/temp_slice2`
- `lang/compiled/temp_tuple`
- `lang/compiled/temp_unpin`
- `lang/compiled/temprepro`
- `lang/compiled/test_audio_module`
- `lang/compiled/test_audio_only`
- `lang/compiled/test_deadlock`
- `lang/compiled/test_path`
- `lang/compiled/test_resample`
- `lang/compiled/test_sslcontext`
- `lang/compiled/thread_probe`
- `lang/compiled/time_crash`
- `lang/compiled/tls12_debug`
- `lang/compiled/tls13_record_roundtrip`
- `lang/compiled/tls384`
- `lang/compiled/tls_bidi_test`
- `lang/compiled/tls_client_debug`
- `lang/compiled/tls_connect_test`
- `lang/compiled/tls_cvtest`
- `lang/compiled/tls_debug`
- `lang/compiled/tls_diag`
- `lang/compiled/tls_diag2`
- `lang/compiled/tls_ecdsa_test`
- `lang/compiled/tls_fix`
- `lang/compiled/tls_large_test`
- `lang/compiled/tls_rsa_gen_test`
- `lang/compiled/tls_test`
- `lang/compiled/tmptest`
- `lang/compiled/totp`
- `lang/compiled/unittest`
- `lang/compiled/universal_test_probe`
- `lang/compiled/unpin_bound_probe`
- `lang/compiled/unpin_generic`
- `lang/compiled/unpin_generic2_probe`
- `lang/compiled/unpin_generic_probe`
- `lang/compiled/ut_macro_probe`
- `lang/compiled/ut_multi_probe`
- `lang/compiled/ut_nested_probe`
- `lang/compiled/ut_runner_probe`
- `lang/compiled/vararg_probe`
- `lang/compiled/varbug`
- `lang/compiled/vecprobe`
- `lang/compiled/vector_set_probe`
- `lang/compiled/view_send`
- `lang/compiled/webview_bind_manual`
- `lang/compiled/webview_native_test`
- `lang/compiled/webview_sample`
- `lang/compiled/webview_window_test`
- `lang/compiled/win_audit`
- `lang/compiled/window_test`
- `lang/compiled/x25519_debug`
- `lang/compiled/x25519_test`
- `lang/compiled/zbench`
- `lang/compiled/zt`

## Library consumers

The test suites (`lang/tests/libs`, `lang/tests/process`, `lang/tests/server`,
`lang/tests/webview`, `lang/tests/universal_webview`, `lang/tests/tls`,
`lang/tests/regexp`, `lang/tests/async`) and the CBI plugins under
`lang/libs/*_cbi` are the library consumers exercised by the Phase 2 gate. The
safe-surface tests are named `safe*_test.ch` / `*_safe_test.ch` and must use
only the new APIs.

## Policy for external/remote users

- A removed syntax form fails with `REMOVED-001` … `REMOVED-006` and the message
  includes the migration hint (see `REMOVED.md`).
- A Phase 2 API break fails with a normal type/symbol error; the migration table
  in `BREAKING.md` names the replacement.
- No migration may insert `unsafe` automatically. A consumer that needs a raw
  escape must opt in explicitly and is outside the safe API guarantee.
