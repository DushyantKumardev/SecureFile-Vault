import React from "react";
import { APP_CONFIG } from "@/constants";
import type { OperationStatus } from "@/types/vault";

interface StatusBarProps {
  status: OperationStatus;
  hasFile: boolean;
}

export const StatusBar: React.FC<StatusBarProps> = ({ status }) => {
  return (
    <footer className="h-8 border-t border-border/70 bg-card/40 px-4 flex items-center justify-between text-xs text-muted-foreground select-none shrink-0">
      <div className="flex items-center gap-2 overflow-hidden">
        <span
          className={`size-1.5 rounded-full shrink-0 ${
            status.loading
              ? "bg-blue-500 animate-pulse"
              : status.type === "error"
                ? "bg-destructive"
                : "bg-emerald-500"
          }`}
        />
        <span className="truncate text-[11px] font-medium text-foreground/80">
          {status.message || APP_CONFIG.status.READY}
        </span>
      </div>

      <span className="text-[10px] text-muted-foreground font-mono hidden sm:inline-block">
        {APP_CONFIG.platform}
      </span>
    </footer>
  );
};
