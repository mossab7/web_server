note: this is just an initial thought about how the projects' structure nothing is certain!

this is how the project is PROBABBLY gonna be structured, this might change in the future:

```
webserv/
├── Makefile               # Build script
├── include/               # Header files
│   ├── server/            # Server core headers
│   │   ├── Server.hpp
│   │   ├── Connection.hpp
│   │   └── EventLoop.hpp
│   ├── config/            # Configuration parsing
│   │   ├── ConfigParser.hpp
│   │   ├── ServerConfig.hpp
│   │   └── RouteConfig.hpp
│   ├── http/              # HTTP protocol handling
│   │   ├── Request.hpp
│   │   ├── Response.hpp
│   │   └── StatusCodes.hpp
│   ├── cgi/               # CGI execution
│   │   └── CGIHandler.hpp
│   └── utils/             # Utilities
│       ├── Logger.hpp
│       ├── FileUtils.hpp
│       ├── SharedPtr.hpp  # To make our life easier
│       └── URI.hpp
├── src/                   # Source files
│   ├── server/
│   │   ├── Server.cpp
│   │   ├── Connection.cpp
│   │   └── EventLoop.cpp
│   ├── config/
│   │   ├── ConfigParser.cpp
│   │   ├── ServerConfig.cpp
│   │   └── RouteConfig.cpp
│   ├── http/
│   │   ├── Request.cpp
│   │   ├── Response.cpp
│   │   └── StatusCodes.cpp
│   ├── cgi/
│   │   └── CGIHandler.cpp
│   └── utils/
│       ├── Logger.cpp
│       ├── FileUtils.cpp
│       └── URI.cpp
├── test/                  # Tests and tools
│   ├── test_configs/      # Sample config files
│   ├── stress_tests/      # Load-testing scripts
│   └── browser_tests/     # HTML/JS files for browser testing
├── configs/               # Default configurations
│   ├── default.conf       # Example config file
│   └── error_pages/       # Default error pages (404.html, 500.html, etc.)
└── docs/                  # Documentation (optional)
    └── DESIGN.md          # Architecture overview
```