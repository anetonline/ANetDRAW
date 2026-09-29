# Door32 unit fixture

The Windows unit runner builds this source as `DOOR32.DLL` beside an isolated
test executable when its manifest configuration requests the fixture.  It
provides the complete Door32 export surface used by `ODComOpen()` so the test
can validate real Windows loader and symbol-resolution behavior on both x86
and x64.

Setting `OPENDOORS_UNIT_DOOR32_INIT_FAIL` makes `DoorInitialize()` fail.  A
case may still wrap `GetProcAddress()` to simulate an individually missing
required export.
