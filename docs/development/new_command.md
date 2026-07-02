# Creating Custom Shell Commands

This tutorial provides a step-by-step walkthrough on how to write, register, and verify a new custom command in the Keira Kernel shell.

---

## Step 1: Create the Command Handler Source File

Create a new Rust file in `kernel/src/shell/cmds/your_command.rs`:

```rust
//! Keira Kernel: Shell Command 'your_command'
//!
//! Implementation of the custom 'your_command' shell utility.

use crate::io::vga;

pub fn run(parts: &mut core::str::SplitWhitespace) {
    // Parse arguments typed by the user
    let arg = parts.next().unwrap_or("default");
    
    vga::print_str("Hello from your new command! Argument: ");
    vga::print_str(arg);
    vga::print_str("\n");
}
```

---

## Step 2: Register the Module in `cmds.rs`

Open `kernel/src/shell/cmds.rs` and add a module declaration:

```rust
pub mod your_command;
```

If your command name is a reserved Rust keyword (like `use` or `move`), escape it using raw identifiers:
```rust
pub mod r#move;
```

---

## Step 3: Add Command Mapping in `executor.rs`

Open `kernel/src/shell/executor.rs`. Navigate to the `execute_command_inner` function and map your command string to your module's `run` function inside the matching block:

```rust
        "your_command" => super::cmds::your_command::run(&mut parts),
```

---

## Step 4: Add to the `Makefile` Command Loops

Open the `Makefile`. To ensure the shell tab-completion and file-check logic detect the new command, append `your_command` to both command lists (in the disk image and initrd populate targets around lines ~148 and ~185):

```makefile
	@for cmd in guide login drives use ramdisk system cpu runtime time memory \
	            devices wait initrd wipe reset run write tasks demo disk list \
	            go script view create folder delete edit say copy help history \
	            move theme your_command please; do \
```

---

## Step 5: Document the Command in `guide.rs`

Open `kernel/src/shell/cmds/guide.rs` and register the usage guidelines under the match block:

```rust
            Some("your_command") => {
                vga::print_str("Usage: your_command [argument]\nThis is a description of my custom command.\n");
            }
```

---

## Step 6: Compile and Verify in QEMU

Rebuild the system to clean stale stubs and compile your new command:
```bash
make clean && make run
```

Once the terminal prints `System ready`, verify your command:
1. Type `guide your_command` to see the help instructions.
2. Type `your_command test_arg` to verify the execution output.
3. Try typing `your_c` and pressing **Tab** to verify autocomplete functions correctly.
