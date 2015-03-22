#include "shell/shell.h"

#include "hal/keyboard.h"
#include "hal/text_ui.h"
#include "kernel/memory.h"
#include "klib/types.h"
#include "klib/strings.h"

using hal::Color;
using hal::TextUI;
using hal::Keyboard::KeyPress;

namespace {

struct ShellCommand {
  const char* command;
  void (*func)(shell::ShellStream* shell);
};

// Initialize shell chrome.
void InitializeChrome();
// Show a map of the machine's memory.
void ShowMemoryMap(shell::ShellStream* shell);
// Returns the command with the name, otherwise null.
const ShellCommand* GetShellCommand(const char* command);

const ShellCommand commands[] = {
  { "show-memory-map", &ShowMemoryMap }
};
const size kNumCommands = sizeof(commands) / sizeof(ShellCommand);

void InitializeChrome() {
  // Title
  for (int i = 0; i < 80; i++) {
    TextUI::SetColor(i, 0, Color::White, Color::Red);
  }
  const char* title = " Goose v1.0a";
  for (int i = 0; i < klib::length(title); i++) {
    TextUI::SetChar(i, 0, title[i]);
  }

  // Rest of the screen.
  for (int y = 1; y < 25; y++) {
    for (int x = 0; x < 80; x++) {
      TextUI::SetChar(x, y, ' ');
      TextUI::SetColor(x, y, Color::Gray, Color::Black);
    }
  }
}

void ShowMemoryMap(shell::ShellStream* shell) {
  const kernel::grub::multiboot_info_t* mbt =
    kernel::GetMultibootInfo();
  kernel::grub::multiboot_memory_map_t* mmap =
    (kernel::grub::multiboot_memory_map*) mbt->mmap_addr;

  size map_idx = 0;
  while (((uint32) mmap) < mbt->mmap_addr + mbt->mmap_length) {
    shell->WriteLine(
        "Memory Map [%d][%p] : size %d, type %d", map_idx, (uint32) mmap,
	mmap->size, mmap->type);
    shell->WriteLine(
        "  address %d / %d", mmap->base_addr_low, mmap->base_addr_high);
    shell->WriteLine(
        "  length  %d / %d", mmap->length_low, mmap->length_high);

    mmap =
        (kernel::grub::multiboot_memory_map_t*)(
            (uint32) mmap + mmap->size + sizeof(uint32));
    map_idx++;
  }
}

const ShellCommand* GetShellCommand(const char* command) {
  for (size i = 0; i < kNumCommands; i++) {
    if (klib::equal(commands[i].command, command)) {
      return &commands[i];
    }
  }
  return nullptr;
}

}  // anonymous namespace

namespace shell {

ShellStream::ShellStream(const hal::Region region, hal::Offset offset) :
  region_(region), offset_(offset) {}

void ShellStream::Print(char c) {
  if (c != '\n') {
    TextUI::SetChar(offset_.x, offset_.y, c);

  // Deal with scrolling.
    offset_.x++;
    if (offset_.x >= region_.offset.x + region_.width) {
      offset_.x = region_.offset.x;
      offset_.y++;
    }
    if (offset_.y >= region_.offset.y + region_.height) {
      TextUI::Scroll(region_);
      offset_.y--;
    }
  }

  if (c == '\n') {
    offset_.x = region_.offset.x;
    offset_.y++;
    if (offset_.y >= region_.offset.y + region_.height) {
      TextUI::Scroll(region_);
      offset_.y--;
    }    
  }
}

hal::Offset ShellStream::Offset() {
  return offset_;
}

void Run() {
  InitializeChrome();
  hal::Region shell_region(0, 1, 80, 24);

  uint8 current_command_line = 1;
  
  const size kMaxCommandLength = 79;  // +1 for null.
  size current_command_idx = 0;
  char current_command[kMaxCommandLength];

  while (true) {
    // Scroll until the command is on the last line.
    while (current_command_line >= 25) {
      TextUI::Scroll(shell_region);
      current_command_line--;
    }

    // Start new command.
    current_command_idx = 0;
    current_command[0] = '\0';

    TextUI::SetChar(0, current_command_line, '$');
    TextUI::SetCursor(2, current_command_line);

    // Read in keystrokes.
    while (true) {
      KeyPress key_press = hal::Keyboard::GetKeypress();
      if (klib::equal(key_press.key.name, "Enter")) {
	break;
      }

      if (klib::equal(key_press.key.name, "Backspace")) {
	if (current_command_idx <= 0) {
	  continue;
	}
	current_command_idx--;
	current_command[current_command_idx] = ' ';
	TextUI::SetChar(current_command_idx + 2, current_command_line, ' ');
	TextUI::SetCursor(current_command_idx + 2, current_command_line);
	continue;
      }

      if (key_press.key.c) {
	TextUI::SetChar(current_command_idx + 2, current_command_line,
			key_press.key.c);

	current_command[current_command_idx] = key_press.key.c;
	current_command_idx++;
	current_command[current_command_idx] = '\0';
	if (current_command_idx >= kMaxCommandLength - 1) {
	  current_command_idx--;
	}

	TextUI::SetCursor(current_command_idx + 2, current_command_line);

	continue;
      }
      // TODO(chrsmith): Delete, and arrow.
    }

    // Process the command.
    current_command_line++;
    while (current_command_line >= 25) {
      TextUI::Scroll(shell_region);
      current_command_line--;
    }

    TextUI::ShowCursor(false);
    hal::Offset shell_offset(0, current_command_line);
    ShellStream stream(shell_region, shell_offset);

    const ShellCommand* command = GetShellCommand(current_command);
    if (klib::equal(current_command, "exit")) {
      return;
    } else if (klib::equal(current_command, "help")) {
      stream.WriteLine("Known Commands:");
      for (size i = 0; i < kNumCommands; i++) {
	stream.WriteLine("  %s", commands[i].command);
      }
    } else if (command == nullptr) {
      stream.WriteLine("Error: Command not found.");
    } else {
      command->func(&stream);
    }

    TextUI::ShowCursor(true);

    // Move the cursor, etc. to where the shell stream finished off.
    hal::Offset offset = stream.Offset();
    current_command_line = offset.y + 1;
  }
}

}  // namespace shell