import { useState, useEffect, useCallback } from "react";
import { Sidebar } from "@/components/Sidebar";
import { FileSelector } from "@/components/FileSelector";
import { CipherForm } from "@/components/CipherForm";
import { ProcessTracker } from "@/components/ProcessTracker";
import { StatusBar } from "@/components/StatusBar";
import { AuditLogs } from "@/components/AuditLogs";
import { SidebarProvider, SidebarInset } from "@/components/ui/sidebar";
import { TooltipProvider } from "@/components/ui/tooltip";
import { APP_CONFIG } from "@/constants";
import { parseFileMetadata, sendToBackend } from "@/lib/bridge";
import type {
  ActiveTab,
  AuditRecord,
  FileMetadata,
  OperationStatus,
  VaultAction,
} from "@/types/vault";

export function App() {
  const [activeTab, setActiveTab] = useState<ActiveTab>(
    APP_CONFIG.tabs.VAULT.id,
  );
  const [selectedFile, setSelectedFile] = useState<FileMetadata | null>(null);
  const [password, setPassword] = useState<string>("");
  const [status, setStatus] = useState<OperationStatus>({
    type: "idle",
    message: APP_CONFIG.status.SYSTEM_READY,
    loading: false,
  });

  const [auditLogs, setAuditLogs] = useState<AuditRecord[]>(() => {
    try {
      const stored = localStorage.getItem(APP_CONFIG.storage.AUDIT_LOGS);
      return stored ? JSON.parse(stored) : [];
    } catch {
      return [];
    }
  });

  /* Append entry to audit history and sync to localStorage */
  const logOperation = useCallback(
    (action: "ENCRYPT" | "DECRYPT", file: FileMetadata, success: boolean) => {
      const newRecord: AuditRecord = {
        id: `${Date.now()}-${Math.random().toString(36).substring(2, 7)}`,
        action,
        fileName: file.fileName,
        fullPath: file.rawPath,
        timestamp: new Date().toLocaleTimeString([], {
          hour: "2-digit",
          minute: "2-digit",
          second: "2-digit",
        }),
        date: new Date().toLocaleDateString(),
        success,
      };

      setAuditLogs((prev) => {
        const updated = [
          newRecord,
          ...prev.slice(0, APP_CONFIG.audit.MAX_RECORDS - 1),
        ];
        try {
          localStorage.setItem(
            APP_CONFIG.storage.AUDIT_LOGS,
            JSON.stringify(updated),
          );
        } catch {
          console.error("Failed to save audit logs");
        }
        return updated;
      });
    },
    [],
  );

  /* Register WebView2 IPC bridge callbacks */
  useEffect(() => {
    window.setFile = (filePath: string) => {
      const meta = parseFileMetadata(filePath);
      setSelectedFile(meta);
      if (meta) {
        setStatus({
          type: "idle",
          message: meta.isEncrypted
            ? `Vault file selected (${meta.fileName}). Provide security key to decrypt.`
            : `Unencrypted file selected (${meta.fileName}). Set key to encrypt in place.`,
          loading: false,
        });
      }
    };

    window.setStatus = (message: string, success: boolean | null) => {
      setStatus({
        type:
          success === true ? "success" : success === false ? "error" : "idle",
        message,
        loading: false,
        timestamp: new Date().toLocaleTimeString(),
      });
    };
  }, []);

  const handleBrowse = useCallback(() => {
    sendToBackend("browse");
  }, []);

  const handleClear = useCallback(() => {
    setSelectedFile(null);
    setStatus({
      type: "idle",
      message: "Selection cleared. System standby.",
      loading: false,
    });
  }, []);

  const handleExecute = useCallback(
    (action: VaultAction) => {
      if (!selectedFile) {
        setStatus({
          type: "error",
          message:
            'Target file required. Click "Browse Storage" to select a file.',
          loading: false,
        });
        return;
      }
      if (!password) {
        setStatus({
          type: "error",
          message: "Cryptographic security key required.",
          loading: false,
        });
        return;
      }

      setStatus({
        type: "loading",
        message:
          action === "encrypt"
            ? "Deriving key and ciphering binary stream..."
            : "Verifying and reversing stream transformation...",
        loading: true,
        action,
      });

      logOperation(
        action.toUpperCase() as "ENCRYPT" | "DECRYPT",
        selectedFile,
        true,
      );
      sendToBackend(action, { file: selectedFile.rawPath, password });
    },
    [selectedFile, password, logOperation],
  );

  const handleClearLogs = useCallback(() => {
    setAuditLogs([]);
    try {
      localStorage.removeItem(APP_CONFIG.storage.AUDIT_LOGS);
    } catch {
      console.error("Failed to clear audit logs");
    }
  }, []);

  return (
    <TooltipProvider delayDuration={150}>
      <SidebarProvider
        defaultOpen={true}
        className="h-screen w-screen flex flex-col bg-background text-foreground antialiased overflow-hidden select-none"
      >
        {/* Top App Layout: Sidebar + Main Workspace */}
        <div className="flex-1 flex overflow-hidden w-full">
          {/* Responsive shadcn Sidebar */}
          <Sidebar
            activeTab={activeTab}
            onTabChange={setActiveTab}
            auditCount={auditLogs.length}
            engineActive={true}
          />

          {/* Main Content Viewport with Ambient Gradient Mesh */}
          <SidebarInset className="flex-1 flex flex-col h-full min-h-0 overflow-hidden relative bg-background">
            {/* Scrollable Workspace Container */}
            <div className="flex-1 overflow-y-auto overscroll-contain relative">
              {/* Subtle Ambient Radial Glow */}
              <div className="pointer-events-none absolute inset-0 bg-[radial-gradient(ellipse_80%_60%_at_50%_-15%,oklch(0.62_0.22_260_/_12%),transparent)] dark:bg-[radial-gradient(ellipse_80%_60%_at_50%_-15%,oklch(0.62_0.22_260_/_18%),transparent)]" />

              <div className="relative max-w-4xl mx-auto p-4 sm:p-6 space-y-6">
                {/* Minimalist Contextual Header */}
                <div className="flex items-center justify-between pb-2 border-b border-border/60">
                  <h1 className="text-lg font-semibold tracking-tight text-foreground">
                    {activeTab === APP_CONFIG.tabs.VAULT.id &&
                      APP_CONFIG.tabs.VAULT.headerTitle}
                    {activeTab === APP_CONFIG.tabs.HISTORY.id &&
                      APP_CONFIG.tabs.HISTORY.headerTitle}
                  </h1>
                </div>

                {/* Tab 1: Vault Operations */}
                {activeTab === "vault" && (
                  <div className="space-y-5">
                    <FileSelector
                      file={selectedFile}
                      onBrowse={handleBrowse}
                      onClear={handleClear}
                      disabled={status.loading}
                    />

                    <CipherForm
                      password={password}
                      onPasswordChange={setPassword}
                      onExecute={handleExecute}
                      isLoading={status.loading}
                      hasFile={!!selectedFile}
                      isEncryptedFile={!!selectedFile?.isEncrypted}
                    />

                    <ProcessTracker status={status} />
                  </div>
                )}

                {/* Tab 2: Audit Trail */}
                {activeTab === "history" && (
                  <AuditLogs logs={auditLogs} onClear={handleClearLogs} />
                )}
              </div>
            </div>

            {/* Atomic StatusBar Inside SidebarInset */}
            <StatusBar status={status} hasFile={!!selectedFile} />
          </SidebarInset>
        </div>
      </SidebarProvider>
    </TooltipProvider>
  );
}

export default App;
