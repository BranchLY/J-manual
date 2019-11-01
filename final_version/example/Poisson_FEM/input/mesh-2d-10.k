$# LS-DYNA Keyword file created by LS-PrePost 4.0 (Beta) - 03Nov2012(16:30)
$# Created on Dec-27-2013 (13:11:59)
*KEYWORD  
*TITLE
$# title
LS-DYNA keyword deck by LS-PrePost
*PART
$# title
LSHELL1
$#     pid     secid       mid     eosid      hgid      grav    adpopt      tmid
         1         0         0         0         0         0         0         0
*SET_NODE_LIST
$#     sid       da1       da2       da3       da4    solver
         1     0.000     0.000     0.000     0.000MECH
$#    nid1      nid2      nid3      nid4      nid5      nid6      nid7      nid8
         3         1         2         5         4         7         6         8
*ELEMENT_SHELL
$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8
       1       1       1      11       3       3       0       0       0       0
       2       1      11       1       5       5       0       0       0       0
       3       1       9      11       5       5       0       0       0       0
       4       1       8       2      10      10       0       0       0       0
       5       1      10       2       3       3       0       0       0       0
       6       1      10       3      11      11       0       0       0       0
       7       1       7       9       5       5       0       0       0       0
       8       1      10      11       9       9       0       0       0       0
       9       1       8      10       9       9       0       0       0       0
      10       1       4       7       5       5       0       0       0       0
      11       1       9       7       6       6       0       0       0       0
      12       1       8       9       6       6       0       0       0       0
*NODE
$#   nid               x               y               z      tc      rc
       1  -7.450581e-009  -7.450581e-009           0.000       0       0
       2  -7.450581e-009        1.000000           0.000       0       0
       3  -7.450581e-009        0.500000           0.000       0       0
       4        1.000000  -7.450581e-009           0.000       0       0
       5        0.500000  -7.450581e-009           0.000       0       0
       6        1.000000        1.000000           0.000       0       0
       7        1.000000        0.500000           0.000       0       0
       8        0.500000        1.000000           0.000       0       0
       9        0.591161        0.590745           0.000       0       0
      10        0.272879        0.689293           0.000       0       0
      11        0.272808        0.356008           0.000       0       0
*END
