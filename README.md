
<h1 align="center">bypass disable_functions via LD_PRELOAD</h1>
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/bruce_lee.jpg" alt=""/><br>
</div>
<hr />

blue guys, I'm sorry, red team wins **agaaaain**.

I know, you disabling all dangerous functions, run operating system commands, such as system() or exec() or shell_exec() and even less known functions such as dl(), through the PHP director defined in php.ini. 

Everybody knows, unix-like supply LD_PRELOAD variable，to provides the possibility of pre-load a share object (*.so) before the others. so I'd like to be able to inject some code into an application. In PHP, the mail() function startups /usr/sbin/sendmail inside, script kiddy hooks some system API in sendmail (such as getuid()) to inject evil code, not bad, **BUT** if bluuuue guys uninstall sendmail tool, now, what you say?

don't worry, I found a new way, whatever sendmail existence, red team always bypass disable_functions via LD_PRELOAD. 

there are two key files in this repository, bypass_disablefunc.php and bypass_disablefunc_x64.so. 

There are three GET options of bypass_disablefunc.php: cmd, outpath and sopath. the options are pretty straightforward, cmd is your evil command, outpath is command output file path (readable and writeable), sopath is the absolute path where our share object bypass_disablefunc_x64.so. For example:
```
http://site.com/bypass_disablefunc.php?cmd=pwd&outpath=/tmp/xx&sopath=/var/www/bypass_disablefunc_x64.so
```

BTW, bypass_disablefunc.php bypasses common anti-webshell tools, **evasion** is always good. so, bypass_disablefunc.php is your friend in any environment.

happy hacking! 
<hr />
 
千辛万苦拿到的 webshell 居然无法执行系统命令：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/%E6%97%A0%E6%B3%95%E6%89%A7%E8%A1%8C%E7%B3%BB%E7%BB%9F%E5%91%BD%E4%BB%A4.png" alt=""/><br>
</div>
怀疑服务端 disable_functions 禁用了命令执行函数，果然如此：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/disable_functions%20%E7%A6%81%E7%94%A8%E5%91%BD%E4%BB%A4%E6%89%A7%E8%A1%8C%E5%87%BD%E6%95%B0.png" alt=""/><br>
</div>
通过环境变量 LD_PRELOAD 劫持系统函数，目标根本没安装 sendmail：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/%E6%97%A0%E6%B3%95%E4%BD%BF%E7%94%A8%20sendmail.png" alt=""/><br>
</div>
无法执行命令的 webshell 是无意义的，得突破！

一般而言，利用漏洞控制 web 启动新进程 a.bin（即便进程名无法让我随意指定），a.bin 内部调用系统函数 b()，b() 位于系统共享对象 c.so 中，所以系统为该进程加载共 c.so，想法在 c.so 前优先加载可控的 c_evil.so，c_evil.so 内含与 b() 同名的恶意函数，由于 c_evil.so 优先级较高，所以，a.bin 将调用到 c_evil.so 内 b() 而非系统的 c.so 内 b()，同时，c_evil.so 可控，达到执行恶意代码的目的。基于这一思路，常见突破 disable_functions 限制执行操作系统命令的方式为：
  * 编写一个原型为 uid_t getuid(void); 的 C 函数，内部执行攻击者指定的代码，并编译成共享对象 getuid_shadow.so；
  * 运行 PHP 函数 putenv()，设定环境变量 LD_PRELOAD 为 getuid_shadow.so，以便后续启动新进程时优先加载该共享对象；
  * 运行 PHP 的 mail() 函数，mail() 内部启动新进程 /usr/sbin/sendmail，由于上一步 LD_PRELOAD 的作用，sendmail 调用的系统函数 getuid() 被优先级更好的 getuid_shadow.so 中的同名 getuid() 所劫持；
  * 达到不调用 PHP 的各种命令执行函数（system()、exec() 等等）仍可执行系统命令的目的。

之所以劫持 getuid()，是因为 sendmail 程序会调用该函数（当然也可以为其他被调用的系统函数），在真实环境中，存在两方面问题：一是，某些环境中，web 禁止启用 senmail、甚至系统上根本未安装 sendmail，也就谈不上劫持 getuid()，通常的 www-data 权限又不可能去更改 php.ini 配置、去安装 sendmail 软件；二是，即便目标可以启用 sendmail，由于未将主机名（hostname 输出）添加进 hosts 中，导致每次运行 sendmail 都要耗时半分钟等待域名解析超时返回，www-data 也无法将主机名加入 hosts（如，127.0.0.1	lamp、lamp.、lamp.com）。基于这两个原因，我不得不放弃劫持函数 getuid()，必须找个更普适的方法。回到 LD_PRELOAD 本身，系统通过它预先加载共享对象，如果能找到一个方式，在加载时就执行代码，而不用考虑劫持某一系统函数，那我就完全可以不依赖 sendmail 了。这种场景与 C++ 的构造函数简直神似！

GCC 有个 C 语言扩展修饰符 `__attribute__((constructor))`，可以让由它修饰的函数在 main() 之前执行，若它出现在共享对象中时，那么一旦共享对象被系统加载，立即将执行 `__attribute__((constructor))` 修饰的函数。这一细节非常重要，很多朋友用 LD_PRELOAD 手法突破 disable_functions 无法做到百分百成功，正因为这个原因，**不要局限于仅劫持某一函数，而应考虑拦劫启动进程这一行为**。

此外，我通过 LD_PRELOAD 劫持了启动进程的行为，劫持后又启动了另外的新进程，若不在新进程启动前取消 LD_PRELOAD，则将陷入无限循环，所以必须得删除环境变量 LD_PRELOAD。最直观的做法是调用 `unsetenv("LD_PRELOAD")`，这在大部份 linux 发行套件上的确可行，但在 centos 上却无效，究其原因，centos 自己也 hook 了 unsetenv()，在其内部启动了其他进程，根本来不及删除 LD_PRELOAD 就又被劫持，导致无限循环。所以，我得找一种比 unsetenv() 更直接的删除环境变量的方式。是它，全局变量 `extern char** environ`！实际上，unsetenv() 就是对 environ 的简单封装实现的环境变量删除功能。

本项目中有三个关键文件，bypass_disablefunc.php、bypass_disablefunc_x64.so、bypass_disablefunc_x86.so。 

bypass_disablefunc.php 为命令执行 webshell，提供三个 GET 参数：
```
http://site.com/bypass_disablefunc.php?cmd=pwd&outpath=/tmp/xx&sopath=/var/www/bypass_disablefunc_x64.so
```
一是 cmd 参数，待执行的系统命令（如 pwd）；二是 outpath 参数，保存命令执行输出结果的文件路径（如 /tmp/xx），便于在页面上显示，另外该参数，你应注意 web 是否有读写权限、web 是否可跨目录访问、文件将被覆盖和删除等几点；三是 sopath 参数，指定劫持系统函数的共享对象的绝对路径（如 /var/www/bypass_disablefunc_x64.so），另外关于该参数，你应注意 web 是否可跨目录访问到它。此外，bypass_disablefunc.php 拼接命令和输出路径成为完整的命令行，所以你不用在 cmd 参数中重定向。

bypass_disablefunc_x64.so 为执行命令的共享对象，用命令 `gcc -shared -fPIC bypass_disablefunc.c -o bypass_disablefunc_x64.so` 将 bypass_disablefunc.c 编译而来。
若目标为 x86 架构，需要加上 -m32 选项重新编译，bypass_disablefunc_x86.so。

想办法将 bypass_disablefunc.php 和 bypass_disablefunc_x64.so 传到目标，指定好三个 GET 参数后，bypass_disablefunc.php 即可突破 disable_functions。执行 `cat /proc/meminfo`：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/%E6%88%90%E5%8A%9F%E7%BB%95%E8%BF%87%20disable_functions.png" alt=""/><br>
</div>
执行 id：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/%E7%AA%81%E7%A0%B4%20disable_functions%20%E6%88%90%E5%8A%9F%E6%89%A7%E8%A1%8C%E5%91%BD%E4%BB%A4.png" alt=""/><br>
</div>

顺便说下，针对 wehshell 的查杀，一般是围绕代码执行函数（如 eval()）、命令执行函数（如 system()）、断言函数（如 assert()）开展的，而 bypass_disablefunc.php 这个 webshell 的本意是突破 disable_functions 执行命令，代码中无任何 webshell 特征函数，所以，副作用是，**它能免杀**。换言之，即便目标并未用 disable_functions 限制命令执行函数，你仍可将 bypass_disablefunc.php 视为普通小马来用，它能躲避后门查杀工具。过 D 盾：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/D%20%E7%9B%BE.png" alt=""/><br>
</div>
过牧云（长亭雷池的开源版）：
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/%E7%89%A7%E4%BA%91.png" alt=""/><br>
</div>
过万国查杀的 YARA（基于特征规则），YARA 提示用户关注，未直接判断为后门:
<div align="center">
<img src="https://github.com/yangyangwithgnu/bypass_disablefunc_via_LD_PRELOAD/blob/master/YARA.png" alt=""/><br>
</div>
<hr />

已知问题：
  * 1）不同目标环境可能需要重新编译共享对象。本项目中的 bypass_disablefunc_x64.so 是在 debian、x64 下编译的，在非 debian 系可能需要重新编译；
  * 2）bypass_disablefunc.php 需要对 outpath 指定的路径具有读写权限，若目标开启 SELinux 可能阻止 web 账号写文件。如，你用 apache 身份运行 id，输出 `uid=48(apache) gid=48(apache) groups=48(apache) context=system_u:system_r:httpd_t:s0`，看到最后的 `context=system_u:system_r:httpd_t:s0` 没，就是 SELinux 在作祟。
<hr />

papers：[《无需 sendmail：巧用 LD_PRELOAD 突破 disable_functions》](https://www.freebuf.com/web/192052.html  "《无需 sendmail：巧用 LD_PRELOAD 突破 disable_functions》")

