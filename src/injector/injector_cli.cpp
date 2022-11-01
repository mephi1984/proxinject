// Copyright 2022 PragmaTwice
//
// Licensed under the Apache License,
// Version 2.0(the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <asio.hpp>

#include "injector.hpp"
#include "injector_cli.hpp"
#include "utils.hpp"
#include "version.hpp"

#include <iostream>

using namespace std;


void inject_by_name(const string& proc_name, injector_server& server, bool& has_process)
{
    auto report_injected = [&has_process](DWORD pid) {
        info("{}: injected", pid);
        has_process = true;
    };

    injector::pid_by_name(proc_name, [&server, &report_injected](DWORD pid) {
        if (server.inject(pid)) {
            report_injected(pid);
        }
    });
}

int main()
{
    string proc_name = "python";

    const bool LOG_ENABLED = false;

    asio::io_context io_context(1);
    injector_server server;

    auto acceptor = tcp::acceptor(io_context, auto_endpoint);
    server.set_port(acceptor.local_endpoint().port());
    info("connection port is set to {}", server.port_);

    asio::co_spawn(io_context,
        listener<injectee_session_cli>(std::move(acceptor), server),
        asio::detached);

    jthread io_thread([&io_context] { io_context.run(); });

    if (LOG_ENABLED) {
        server.enable_log();
        info("logging enabled");
    }


    server.enable_subprocess();
    info("subprocess injection enabled");
    

    string addr = "127.0.0.1";

    uint32_t port = 8043;

    server.set_proxy(ip::address::from_string(addr), port);
    info("proxy address set to {}:{}", addr, port);

    bool has_process = false;

    inject_by_name(proc_name, server, has_process);

    while (!has_process)
    {
        std::this_thread::sleep_for(1000ms);
        info("Waiting for the process to inject...");
        inject_by_name(proc_name, server, has_process);
    };

    info("Process was successfully injected.");

 
    /*
    if (!has_process) {
        info("no process has been injected, exit");
        io_context.stop();
    }*/

    io_thread.join();

}
