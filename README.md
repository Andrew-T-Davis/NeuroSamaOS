# NeuroSamaOS

> **An operating system. From scratch. On real hardware. With music.**

NeuroSamaOS is a 64-bit UEFI operating system I built by reading Intel manuals and writing every line of code myself. It boots on real PCs, draws graphics, plays WAV audio, reads FAT32 drives, and even runs JIT-compiled code at runtime.

**It's not a Linux clone. It's not a toy. It's my OS. Can Run In Real CPU**

**Running on a laptop is strongly discouraged, Because it consumes electricity, or if you want laptop , pls use this system at home pls**

---

## What it can do (right now)

| What | How it looks to you |
|------|---------------------|
| **Boots on real UEFI computers** | No emulator needed. Burn it to USB and go. |
| **Graphical display (GOP)** | Color, pixels, not just text. |
| **Plays WAV music** | Speakers work. AC97 and HDAudio supported. |
| **Reads your hard drive** | FAT32 file system. ATA driver. GPT support. |
| **Keyboard + Mouse** | PS/2 devices work. |
| **JIT Compiler** | Writes and runs machine code at runtime. |
| **AVX256 / SSE / FPU** | Uses modern CPU instructions. |
| **Window Manager** | Real windows, not just fullscreen or Text Mode, its Graphical Mode. |

---

## under development

| What | How it looks to you |
|------|---------------------|
| **HolyC-JIT** | You can write HolyC-JIT code in NeuroSamaOS, not in TempleOS. |
| **HolyNeuro-JIT** | write code easily, clearly and fast. |
| **Lua-JIT** | Make it so that people who don't know either of the two programming languages ​​can write code easily, clearly, and fast. |

---

## What's being built

- **xHCI (USB 3.0)** – So you can use USB keyboards/mice
- **E1000 Ethernet And IwlWiFi** – So it can talk to the internet

---

## How it was built

- **Language:** C + GAS
- **Boot:** UEFI (Not BIOS)
- **Reference:** Intel manuals only. No Linux code. No Minix code.
- **Time:** Months of nights and weekends.
- **Lines:** Thousands. All mine.
- **GPU:** Any, But No PixelBltOnly.
- **CPU:** Need AVX256, SSE And FPU.
- **Memory:** Dont Less Than 2GB(If Less Then 2GB Will Crash). Recommend 4GB Or More Then 4GB

---


## Build & Run

```bash
# Build
make
```

### Run

| Speed | Backend | xHCI (USB 3.0) | PS/2 |
|------|------|---------------|------|
| **Portable** (works everywhere) | TCG | `make run-tcg` | `make run-tcg-ps2` |
| **Fast** (Linux KVM) | KVM | `make run-kvm` | `make run-kvm-ps2` |
| **Faster** (KVM + host CPU passthrough) | KVM + `-cpu host` | `make run-speed-kvm` | `make run-speed-kvm-ps2` |`

### Debug

| Speed | Backend | xHCI (USB 3.0) | PS/2 |
|------|------|---------------|------|
| Portable | TCG | `make debug-tcg` | `make debug-tcg-ps2` |
| Fast | KVM | `make debug-kvm` | `make debug-kvm-ps2` |

---

## Acknowledgments

- **Terry A. Davis** – My Spiritual Mentor
- **Intel Manuals** – The only documentation I used.

---

## License

MIT – Use it, break it, fix it, fork it.

--

# PS

(I didn't browse the OSDev Wiki or any Linux source code at all during development. Btw)

---