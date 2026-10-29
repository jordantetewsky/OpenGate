// SPDX-License-Identifier: MIT
import AppKit

let directory = ProcessInfo.processInfo.environment["UNBOUND_LAUNCH_DIRECTORY"].map { URL(fileURLWithPath: $0) }
    ?? FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent("Library/Logs/OpenGate")

func record(_ event: String, details: [String: Any] = [:]) {
    do {
        try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
        var row = details
        row["component"] = "OpenGate"; row["vendor"] = "CineWaltz"
        row["event"] = event
        row["time"] = ISO8601DateFormatter().string(from: Date())
        row["pid"] = ProcessInfo.processInfo.processIdentifier
        let path = directory.appendingPathComponent("launches.jsonl")
        if !FileManager.default.fileExists(atPath: path.path) { FileManager.default.createFile(atPath: path.path, contents: nil) }
        let file = try FileHandle(forWritingTo: path)
        defer { try? file.close() }
        try file.seekToEnd()
        try file.write(contentsOf: JSONSerialization.data(withJSONObject: row) + Data([10]))
    } catch { NSLog("OpenGate logging: %@", error.localizedDescription) }
}

@discardableResult func requestControls() throws -> String {
    try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
    let token = "app-" + UUID().uuidString
    try token.write(to: directory.appendingPathComponent("show-controls.request"), atomically: true, encoding: .utf8)
    record("app.controls.request", details: ["request": token])
    return token
}

if CommandLine.arguments.contains("--self-test") {
    do {
        let first = try requestControls(), second = try requestControls()
        let actual = try String(contentsOf: directory.appendingPathComponent("show-controls.request"), encoding: .utf8)
        guard first != second && actual == second else { exit(1) }
        print("PASS: repeatable launcher requests and diagnostic logging")
        exit(0)
    } catch { fputs("Launcher self-test failed: \(error)\n", stderr); exit(1) }
}

final class Launcher: NSObject, NSApplicationDelegate {
    private var window: NSWindow!
    private let status = NSTextField(wrappingLabelWithString: "Open Resolve and a project to use panel controls.")

    func applicationDidFinishLaunching(_ notification: Notification) {
        record("app.launch")
        window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 420, height: 240),
                          styleMask: [.titled, .closable, .miniaturizable], backing: .buffered, defer: false)
        window.title = "OpenGate"
        window.isReleasedWhenClosed = false
        let show = NSButton(title: "Show Panel Controls", target: self, action: #selector(showControls))
        let logs = NSButton(title: "Open Diagnostic Logs", target: self, action: #selector(openLogs))
        let credit = NSTextField(wrappingLabelWithString: "OpenGate by CineWaltz. Built on Unbound (MIT).")
        let stack = NSStackView(views: [status, show, logs, credit])
        stack.orientation = .vertical
        stack.alignment = .leading
        stack.spacing = 16
        stack.translatesAutoresizingMaskIntoConstraints = false
        window.contentView!.addSubview(stack)
        NSLayoutConstraint.activate([
            stack.leadingAnchor.constraint(equalTo: window.contentView!.leadingAnchor, constant: 20),
            stack.trailingAnchor.constraint(equalTo: window.contentView!.trailingAnchor, constant: -20),
            stack.topAnchor.constraint(equalTo: window.contentView!.topAnchor, constant: 20)
        ])
        window.center()
        window.makeKeyAndOrderFront(nil)
        requestAndOpen()
    }

    @objc func showControls() {
        record("app.button", details: ["action": "show-controls"])
        requestAndOpen()
    }
    private func requestAndOpen() {
        do {
            try requestControls()
            if NSRunningApplication.runningApplications(withBundleIdentifier: "com.blackmagic-design.DaVinciResolve").isEmpty {
                let app = URL(fileURLWithPath: "/Applications/DaVinci Resolve/DaVinci Resolve.app")
                NSWorkspace.shared.openApplication(at: app, configuration: NSWorkspace.OpenConfiguration()) { _, error in
                    DispatchQueue.main.async {
                        if let error = error {
                            self.status.stringValue = error.localizedDescription
                            record("app.resolve.launch.failed", details: ["error": error.localizedDescription])
                        } else { record("app.resolve.launch") }
                    }
                }
            }
            status.stringValue = "Controls requested. Resolve’s installed helper will show them when ready."
        } catch {
            status.stringValue = error.localizedDescription
            record("app.controls.failed", details: ["error": error.localizedDescription])
        }
    }
    @objc func openLogs() {
        record("app.button", details: ["action": "open-logs"])
        NSWorkspace.shared.open(directory)
    }
    func applicationShouldHandleReopen(_ sender: NSApplication, hasVisibleWindows flag: Bool) -> Bool {
        record("app.reopen")
        window.makeKeyAndOrderFront(nil)
        requestAndOpen()
        return true
    }
    func applicationWillTerminate(_ notification: Notification) { record("app.quit") }
}

let app = NSApplication.shared
let delegate = Launcher()
app.setActivationPolicy(.regular)
app.delegate = delegate
app.run()
