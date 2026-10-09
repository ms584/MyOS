# VM1024 focused candidate — NOT Phase33.4 acceptance
Native BIOS+UEFI >256 reservation saturation, atomic full-table split rejection,
capacity reuse, split/merge at cap, fragmented hole reuse, middle/high pthread
lifecycle and exact resource cleanup passed twice each. Capacity bounded1024,
metadata16KB/process dynamically allocated. Scalar ENOMEM diagnostics serial
only, at most2/process. Original failure returns preserved. New full regression
required/pending; prior PMM1GiB full does not cover this source change.
Native online diagnostic started00:38:45; no pass claimed.
