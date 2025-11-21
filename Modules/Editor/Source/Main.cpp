#include <Editor.hpp>
#include <spdlog/sinks/basic_file_sink.h>

int32_t main(
    const int32_t argc,
    const char  **argv) {
    Ignis::Editor editor{};

    editor.initialize(argc, argv);
    editor.run();
    editor.release();

    return 0;
}