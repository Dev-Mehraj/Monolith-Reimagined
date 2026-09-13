# AI Panel Integration Guide

## Overview
The Monolith Reimagined IDE now includes a powerful AI panel powered by **llama.cpp**, enabling local LLM inference with full agentic capabilities via JSON tool calling.

## Features

### 🧠 Agentic AI with Tool Calling
- **JSON-based tool definitions** following OpenAI-compatible format
- **Automatic tool execution** when the model requests tools
- **Multi-turn conversations** with tool result feedback
- **Streaming responses** for real-time token display

### 🛠️ Built-in IDE Tools
The AI panel comes with 9 pre-configured tools:

| Tool | Description |
|------|-------------|
| `read_file` | Read file contents from workspace |
| `write_file` | Create or overwrite files |
| `search_in_files` | Ripgrep-powered text search |
| `list_directory` | List directory contents |
| `run_terminal_command` | Execute shell commands |
| `get_editor_selection` | Get selected text from editor |
| `insert_code_at_cursor` | Insert AI-generated code |
| `get_file_diagnostics` | Fetch LSP errors/warnings |
| `get_git_status` | Check Git file status |

### 💬 Chat Interface
- **Message bubbles** with markdown rendering
- **Code blocks** with syntax highlighting, copy & insert buttons
- **Tool call indicators** showing execution status
- **Settings panel** for model/temperature/max tokens configuration
- **Chat export/import** in Markdown format

## Dependencies

### Downloaded (in `deps/` folder)
- ✅ **llama.cpp** - Local LLM inference engine
- ✅ **nlohmann_json** - JSON parsing for tool calls
- ✅ **tree_sitter** - AST parsing for code understanding
- ✅ **libgit2** - Git operations
- ✅ **duktape** - JavaScript runtime for extensions
- ✅ **libvterm** - Terminal emulation

## Building

### Prerequisites
```bash
# Qt6 development packages
sudo apt install qt6-base-dev qt6-svg-dev qt6-network-dev

# Build tools
sudo apt install cmake ninja-build build-essential

# Optional: CUDA for GPU acceleration
sudo apt install nvidia-cuda-dev
```

### Build Commands
```bash
cd /workspace
mkdir -p build && cd build

# Configure with CMake
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_AI=ON \
    -DENABLE_GIT=ON \
    -DENABLE_TERMINAL=ON \
    -DENABLE_EXTENSIONS=ON

# Build
ninja

# The llama-server binary will be in deps/llama_cpp/build/bin/
```

## Usage

### Loading a Model
1. Download a GGUF model (e.g., from HuggingFace):
   ```bash
   # Example: Llama-3.2-3B-Instruct
   wget https://huggingface.co/bartowski/Llama-3.2-3B-Instruct-GGUF/resolve/main/Llama-3.2-3B-Instruct-Q4_K_M.gguf
   mkdir -p ~/.monolith/models
   mv Llama-3.2-3B-Instruct-Q4_K_M.gguf ~/.monolith/models/
   ```

2. In the AI Panel settings, select your model path

3. Click "Save Settings" to load the model

### Example Conversations

#### Code Generation
```
User: Create a Python function that sorts a list using quicksort

AI: [Generates code block with quicksort implementation]
     [Insert] [Copy] buttons appear on code block
```

#### File Operations (Agentic)
```
User: Create a new file called hello.py with a print statement

AI: 🔧 Calling tool: write_file
    [Tool executes, creates file]
    Done! I've created hello.py with: print("Hello, World!")
```

#### Code Analysis
```
User: What's wrong with this code? [selects code in editor]

AI: 🔧 Calling tool: get_editor_selection
    🔧 Calling tool: get_file_diagnostics
    I found 2 issues:
    1. Missing semicolon on line 5
    2. Unused variable 'x' on line 10
```

## Architecture

### Key Classes

#### `LlamaClient` (`src/ai/LlamaClient.h`)
- Manages llama.cpp server process
- Handles JSON-RPC communication
- Parses SSE streaming responses
- Manages tool registration and execution

#### `AIPanel` (`src/ai/AIPanel.h`)
- Main UI widget for chat interface
- Renders message bubbles with markdown
- Displays tool call widgets
- Provides settings panel

#### Tool Structures
```cpp
struct ToolDefinition {
    QString name;
    QString description;
    QJsonObject parameters; // JSON Schema
};

struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
};

struct ToolResult {
    QString toolCallId;
    QString content;
    bool isError;
};
```

## API Reference

### Registering Custom Tools
```cpp
ToolDefinition myTool;
myTool.name = "custom_tool";
myTool.description = "Does something useful";
myTool.parameters = QJsonObject{
    {"type", "object"},
    {"properties", QJsonObject{
        {"param1", QJsonObject{
            {"type", "string"},
            {"description", "First parameter"}
        }}
    }},
    {"required", QJsonArray{"param1"}}
};

llamaClient->registerTool(myTool);
```

### Sending Messages
```cpp
// Simple message
aiPanel->sendMessage("Hello, AI!");

// With tool context
ChatMessage msg;
msg.role = "user";
msg.content = "Fix this bug";
llamaClient->sendRequest({msg}, true); // enableTools = true
```

### Handling Tool Execution
```cpp
connect(aiPanel, &AIPanel::executeToolRequested,
        this, [](const ToolCall& call) {
    if (call.name == "read_file") {
        QString path = call.arguments["path"].toString();
        QString content = readFile(path);
        
        ToolResult result;
        result.toolCallId = call.id;
        result.content = content;
        
        aiPanel->onToolExecuted(result);
    }
});
```

## Configuration Options

### LlamaConfig
```cpp
LlamaConfig config;
config.modelPath = "/path/to/model.gguf";
config.nContext = 4096;      // Context window size
config.nBatch = 512;         // Batch size
config.nThreads = 4;         // CPU threads
config.nGpuLayers = 35;      // GPU offload layers (0 = CPU only)
config.temperature = 0.7f;   // Creativity vs determinism
config.topP = 90;            // Nucleus sampling
config.topK = 40;            // Top-K sampling
config.maxTokens = 2048;     // Max generation length
```

## Troubleshooting

### Model fails to load
- Ensure GGUF format model is downloaded
- Check file permissions
- Verify model path in settings

### Slow inference
- Enable GPU acceleration: increase `nGpuLayers`
- Use smaller quantized models (Q4_K_M recommended)
- Reduce `nContext` if memory is limited

### Tool calls not working
- Ensure `enableTools=true` in `sendRequest()`
- Check tool JSON schema is valid
- Verify tool execution handler is connected

## Performance Tips

1. **Use quantized models**: Q4_K_M offers best speed/quality balance
2. **GPU offloading**: Set `nGpuLayers` to max your VRAM allows
3. **Batching**: Increase `nBatch` for faster prompt processing
4. **Context management**: Clear chat history for long sessions

## Security Considerations

⚠️ **Important**: The `run_terminal_command` tool can execute arbitrary commands. Consider:
- Adding confirmation dialogs for dangerous operations
- Whitelisting allowed commands
- Running in sandboxed environment
- Disabling tool by default in production

## Future Enhancements

- [ ] RAG (Retrieval Augmented Generation) with embeddings
- [ ] Multi-model support with fallback
- [ ] Vision models for screenshot analysis
- [ ] Voice input/output
- [ ] Collaborative AI sessions
- [ ] Extension marketplace for custom tools
