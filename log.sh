lstrand@styx:~/dv/spike$ bzr fast-export --plain . | git fast-import
07:57:40 Calculating the revisions to include ...
07:57:40 Starting export of 179 revisions ...
07:57:44 Exported 179 revisions in 0:00:05
/usr/lib/git-core/git-fast-import statistics:
---------------------------------------------------------------------
Alloc'd objects:       5000
Total objects:         2458 (        10 duplicates                  )
      blobs  :         1940 (        10 duplicates        997 deltas of       1931 attempts)
      trees  :          339 (         0 duplicates        311 deltas of        336 attempts)
      commits:          179 (         0 duplicates          0 deltas of          0 attempts)
      tags   :            0 (         0 duplicates          0 deltas of          0 attempts)
Total branches:           2 (         1 loads     )
      marks:           1024 (       179 unique    )
      atoms:            247
Memory total:          2344 KiB
       pools:          2110 KiB
     objects:           234 KiB
---------------------------------------------------------------------
pack_report: getpagesize()            =       4096
pack_report: core.packedGitWindowSize = 1073741824
pack_report: core.packedGitLimit      = 35184372088832
pack_report: pack_used_ctr            =          5
pack_report: pack_mmap_calls          =          2
pack_report: pack_open_windows        =          1 /          1
pack_report: pack_mapped              =    3706847 /    3706847
---------------------------------------------------------------------

lstrand@styx:~/dv/spike$ 

