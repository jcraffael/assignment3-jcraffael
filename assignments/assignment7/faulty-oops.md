root@qemuarm64:~# echo “hello_world” > /dev/faulty
[   80.516814] Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
[   80.521172] Mem abort info:
[   80.521299]   ESR = 0x0000000096000045
[   80.521488]   EC = 0x25: DABT (current EL), IL = 32 bits
[   80.521644]   SET = 0, FnV = 0
[   80.521737]   EA = 0, S1PTW = 0
[   80.521832]   FSC = 0x05: level 1 translation fault
[   80.521967] Data abort info:
[   80.522051]   ISV = 0, ISS = 0x00000045
[   80.522152]   CM = 0, WnR = 1
[   80.522357] user pgtable: 4k pages, 39-bit VAs, pgdp=00000000435f0000
[   80.522544] [0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
[   80.523821] Internal error: Oops: 0000000096000045 [#1] PREEMPT SMP
[   80.524195] Modules linked in: scull(O) faulty(O)
[   80.524928] CPU: 1 PID: 439 Comm: sh Tainted: G           O      5.15.194-yocto-standard #1
[   80.525214] Hardware name: linux,dummy-virt (DT)
[   80.525482] pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[   80.525739] pc : faulty_write+0x18/0x20 [faulty]
[   80.526692] lr : vfs_write+0xf8/0x2a0
[   80.527111] sp : ffffffc00c303d80
[   80.527188] x29: ffffffc00c303d80 x28: ffffff8003762940 x27: 0000000000000000
[   80.527398] x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
[   80.527537] x23: 0000000000000000 x22: ffffffc00c303dc0 x21: 000000557095b9b0
[   80.527674] x20: ffffff800361dd00 x19: 0000000000000012 x18: 0000000000000000
[   80.527814] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
[   80.527954] x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
[   80.528098] x11: 0000000000000000 x10: 0000000000000000 x9 : ffffffc008272108
[   80.528263] x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
[   80.528397] x5 : 0000000000000001 x4 : ffffffc000ba0000 x3 : ffffffc00c303dc0
[   80.528533] x2 : 0000000000000012 x1 : 0000000000000000 x0 : 0000000000000000
[   80.528824] Call trace:
[   80.528928]  faulty_write+0x18/0x20 [faulty]
[   80.529066]  ksys_write+0x74/0x110
[   80.529142]  __arm64_sys_write+0x24/0x30
[   80.529221]  invoke_syscall+0x5c/0x130
[   80.529305]  el0_svc_common.constprop.0+0x4c/0x100
[   80.529398]  do_el0_svc+0x4c/0xc0
[   80.529468]  el0_svc+0x28/0x80
[   80.529537]  el0t_64_sync_handler+0xa4/0x130
[   80.529620]  el0t_64_sync+0x1a0/0x1a4
[   80.529860] Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
[   80.530206] ---[ end trace 0faf0c14f0e64d04 ]---
Segmentation fault
