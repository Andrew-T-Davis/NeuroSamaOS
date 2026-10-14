(抱歉有点手册后遗症，也就是习惯了手册的讲解方式)

Neuro-SamaOS —— 一个以Neuro-Sama命名的纯裸机不基于Linux和Windows而且纯Ring0的UEFI操作系统

这个系统顺便致敬TempleOS

使用体验怎么样？对于普通用户我不确定，但对于程序员，我敢肯定有许多便利之处

为什么？

因为这个系统他内置JIT后端和AST前端，自由到什么地步呢？

你需要自己填写JIT这个操作会做什么，比如就拿AOT来说，你可以直接定义一个Keyword:
```c
typedef struct Keyword Keyword;
struct Keyword
{
    char* Name;
    u16 Type;
    void (*ToJITDo)(Keyword Keyword, ASTNode* ASTNode);
} __attribute__((packed));
```
什么？你说你要定义语法？没问题！SyntaxRule帮你搞定
```c
struct SyntaxRule
{
    char* Name;
    char* Pattern;
    CaptureNode* Captures;
    ASTNode* Body;
    void (*EmitCustom)(void* node, void* dsl, void* p);
} __attribute__((packed));
```
你想定义指针符号？没问题！
```c
typedef struct
{
    u64 KeywordCount;
    Keyword** Keywords;
    u64 RuleCount;
    SyntaxRule** Rules;
    char PointerSymbol;
    char AddressSymbol;
    char DerefSymbol;
} __attribute__((packed)) FishDSL;
```
你想添加他们？没问题！
```c
void AddKeyword(Keyword* KeyWord, FishDSL* DSL);
void AddSyntaxRule(SyntaxRule* Rule, FishDSL* DSL);
void AddChild(ASTNode* parent, ASTNode* child);
```
写错了代码？

IDT帮你兜底，Neuro-SamaOS的IDT内置CISC长度解析器，

还可以解析VEX系列，然后操控RIP寄存器帮你跳过，不过会在桌面弹出一个必须要手动点击关闭的警告，因为这真的至关重要

然后，恭喜你，一个新的语言被你亲手创造出来了

说了那么多，终端呢？

这个系统终端是严格大小写的，一切都要大驼峰（相信Python或者Java或者C#程序员很熟悉）

这个系统有的指令：
        Help - Show Help

        Clear - clear screen

        Echo XXX - Print XXX

        ShutDown - shutdown

        ReBoot - reboot

        Ver - show version

要什么配置才能运行？

随便找一个有AVX256和SSE的CPU,然后找一个亮机卡，或者不需要GPU就能显示的CPU,就可以运行

GPU有条件吗？

有，不过这个大多数都不会被这个条件拦下，也就是PixelBltOnly,如果你的GPU是PixelBltOnly，

那么我的操作系统不欢迎你，

如果你的GPU是BltOnly,那么我的OS会显示：
```text
You Fucking GPU Is Fucking PixelBltOnly Bro,
Dont Use This Fucking Shit GPU, Buy A New One Pls Bro
Press A: Return To UEFI Boot Menu
Press B: Try Next Boot Options
Press Any Other Key: Shutdown
```

是的，他就这样

隔壁微软连报错都不给，直接黑屏，他们也一样不支持PixelBltOnly

你问文件系统？这个系统用FAT32和ATA,SATA目前还在开发

我用的是协助式多任务，所以我的while都是编译成TaskAdd，那么break呢？直接TaskDel

这个系统不是BMP,也不是SMP,这个系统是AMP,也就是核心0给其他核心发送消息然后做核心0分配的事情

你说你不是程序员？那么自求多福吧，我这个系统我目前我还不太确定对普通用户怎么样，我希望你能反馈

这个系统既不是纯轮询也不是纯中断，这个系统是中断驱动（也就是WaitForEvent()）TaskPoll轮询，这样大大降低了复杂性也可以让你们的CPU不过热

栈的话，总共288MB,是动态切割的，比如你只有一个核心，那么你独享整整288MB的栈，两个核心就每一个144MB,以此类推，288个核心就是每一个1MB栈，好心吧？

这个系统纯Ring0内核态，很好玩，有网络，E1000,iwlwifi这两个我在尝试（说实话我讨厌iwlwifi,因为需要手动固件而不是内部处理很多，iwlwifi就是垃圾，但是偏偏我就在用，呜呜呜呜呜呜，还是Wifi6，呜呜呜呜呜呜，我房间还没有网线，呜呜呜呜呜呜：
```bash
[andy@andy-b760mhdvm2 NeuroSamaOS]$ lspci | grep -i net
02:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8125 2.5GbE Controller (rev 05)
03:00.0 Network controller: Intel Corporation Wi-Fi 6 AX200 (rev 1a)
[andy@andy-b760mhdvm2 NeuroSamaOS]$ 
```
）

我索性列一个列表出来吧（以下全部都是我的比喻）：

- xHCI - USB 3.x驱动
- HDAudio - 现代音频驱动
- AC97 - 以前的音频驱动
- GPT - GPT分区
- GOP - 显示用的
- FAT32 - 磁盘格式驱动
- SATA - 底层的底层，GPT和FAT32都是建立在他的上面，没有SATA/ATA就没有这两个
- ACPI - 关机和重启和HPET
- HPET - 高精度ACPI计时器
- PCI/PCIe - 没有PCI/PCIe就没有HDAudio和AC97和xHCI
- PS/2 - 以前的鼠标和键盘

等我写完xHCI初始化和他的逻辑我就能开始写AHCI和SATA了

然后呢，我觉得ATA没用了，所以我移除了ATA,然后我也不用标准的FAT32/Ext,我要转TempleOS的RedSea和我自己的FDFD格式了

就这样啦，再见！