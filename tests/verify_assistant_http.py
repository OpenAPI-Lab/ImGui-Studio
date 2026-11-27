import json
import subprocess
import sys
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


PROPOSAL = '```imgui-json\n{"type":"window","label":"Test"}\n```'
PROVIDER_MODELS = {
    "ollama": ("/api/tags", {"models": [{"name": "fixture"}]}),
    "lmstudio": ("/v1/models", {"data": [{"id": "fixture"}]}),
    "openai": ("/v1/models", {"data": [{"id": "fixture"}]}),
    "anthropic": ("/v1/models", {"data": [{"id": "fixture"}]}),
    "gemini": ("/v1beta/models", {"models": [{"name": "models/fixture"}]}),
}


class Fixture(BaseHTTPRequestHandler):
    def respond(self, body, status=200):
        encoded = json.dumps(body).encode()
        try:
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(encoded)))
            self.end_headers()
            self.wfile.write(encoded)
        except (BrokenPipeError, ConnectionResetError):
            pass  # Expected when the client cancels or enforces the response limit.

    def check_auth(self, provider):
        header = {"anthropic": "x-api-key", "gemini": "x-goog-api-key"}.get(provider, "Authorization")
        expected = "Bearer mock-key" if header == "Authorization" else "mock-key"
        assert self.headers.get(header) == expected
        if provider == "anthropic":
            assert self.headers.get("anthropic-version") == "2023-06-01"

    def do_GET(self):
        provider, route = self.path.lstrip("/").split("/", 1)
        if provider == "redirect":
            self.send_response(307)
            self.send_header("Location", "/openai/v1/models")
            self.end_headers()
            return
        if provider == "denied":
            return self.respond({"error": "provider detail containing mock-key"}, 401)
        if provider == "large":
            return self.respond({"oversized": "x" * (2 * 1024 * 1024)})
        if provider == "slow":
            time.sleep(6)
            return self.respond({"data": []})
        expected, response = PROVIDER_MODELS[provider]
        self.check_auth(provider)
        assert "/" + route == expected
        self.respond(response)

    def do_POST(self):
        provider, route = self.path.lstrip("/").split("/", 1)
        self.check_auth(provider)
        body = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
        if provider == "gemini":
            assert route == "v1beta/models/fixture:generateContent"
            assert body["systemInstruction"]["parts"][0]["text"] == "Instruccion"
            assert [row["role"] for row in body["contents"]] == ["user", "model", "user"]
            return self.respond({"candidates": [{"content": {"parts": [{"text": PROPOSAL}]}}]})
        assert body["model"] == "fixture"
        assert body["messages"][-1]["content"] == "Consulta final"
        assert body["stream"] is False
        if provider == "anthropic":
            assert route == "v1/messages" and body["system"] == "Instruccion"
            assert body["max_tokens"] > 0 and body["messages"][0]["role"] == "user"
            return self.respond({"content": [{"type": "text", "text": PROPOSAL}]})
        assert body["messages"][0] == {"role": "system", "content": "Instruccion"}
        if provider == "ollama":
            assert route == "api/chat"
            return self.respond({"message": {"content": PROPOSAL}})
        assert route == "v1/chat/completions"
        self.respond({"choices": [{"message": {"content": PROPOSAL}}]})

    def log_message(self, *_args):
        pass


with ThreadingHTTPServer(("127.0.0.1", 0), Fixture) as server:
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        subprocess.run([sys.argv[1], f"http://127.0.0.1:{server.server_port}"], check=True, timeout=25)
    finally:
        server.shutdown()
