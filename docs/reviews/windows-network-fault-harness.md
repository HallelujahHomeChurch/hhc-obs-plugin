# Developer-only HHC HTTP outage test

Configure a separate build with `-DHHC_NETWORK_TEST_FAULTS=ON`. Set
`HHC_TEST_NETWORK_OFFLINE_FILE` in only the isolated OBS process environment
to an absolute marker path. Its upload helper inherits that environment.
While the marker exists, all HHC HTTP requests fail with
`transport_unavailable`; in-flight requests abort within the existing 100 ms
cancellation poll. Remove the marker to restore normal HTTP. Encoding and
the durable local queue continue through the ordinary recovery path.

This simulates a transport failure, not a physical network outage, packet
loss, DNS failure or TLS timeout. It does not affect the computer's network,
the browser, global OBS streaming or YouTube. Do not claim physical-network
acceptance from this test.

The option defaults OFF. The normal HTTP library ignores the environment
variable entirely. The fault helper reports a `-network-faults` build suffix,
and OBS logs a test-build warning. Packaging rejects fault-enabled builds and
requires the helper's exact source revision. Never distribute this variant.

Automated loopback tests cover blocking new requests, aborting an in-flight
upload, restoration, and the normal build ignoring the marker. Before a
field test, retain the installed candidate binaries and record the fault
build's source revision and hashes separately. Restore the normal candidate
after the test, with isolated OBS and its helper stopped.
