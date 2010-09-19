#!/bin/bash

# Remove OpenITG's Patch.rsa, which currently overrides ITG's Patch.rsa.
# This allows us to use the Patch-OpenITG.rsa in patch.zip as well, so
# we can install both ITG2 and OpenITG patches.

if [ -f /stats/Patch.rsa ]; then
	rm /stats/Patch.rsa
fi

exit 0
