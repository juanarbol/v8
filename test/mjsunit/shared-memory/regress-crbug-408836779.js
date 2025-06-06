// Copyright 2025 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Flags: --expose-gc --minor-ms --shared-heap

function f() {
  var x = new Uint32Array();
  try {
    f();
  } catch {}
}
for (var i = 0; i < 10; i++) {
  new Worker(f, {type: 'function'});
  gc();
}
