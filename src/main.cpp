#include <miniengine/app.h>

#include <spdlog/spdlog.h>

int main(void) {
  spdlog::set_level(spdlog::level::trace);

  spdlog::info("Starting MiniEngine");

  MiniEngine::App app;

  try {
    app.Run();
  } catch (const std::exception &e) {
    spdlog::error("Exception: {}", e.what());
    return EXIT_FAILURE;
  }

  spdlog::info("Shutting down MiniEngine");

  return EXIT_SUCCESS;
}
