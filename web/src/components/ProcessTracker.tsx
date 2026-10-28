import React from "react";
import { Loader2, AlertTriangle } from "lucide-react";
import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";
import { APP_CONFIG } from "@/constants";
import type { OperationStatus } from "@/types/vault";

interface ProcessTrackerProps {
  status: OperationStatus;
}

export const ProcessTracker: React.FC<ProcessTrackerProps> = ({ status }) => {
  const { type, message, loading, action } = status;

  /* If idle or successfully completed, let the bottom StatusBar be the single clean indicator */
  if ((type === "idle" || type === "success") && !loading) {
    return null;
  }

  return (
    <div className="space-y-3 transition-all animate-in fade-in duration-200">
      {/* Loading State */}
      {loading && (
        <div className="p-4 rounded-xl border border-primary/30 bg-primary/5 flex items-center gap-3 shadow-xs">
          <Loader2 className="size-4 text-primary animate-spin shrink-0" />
          <div className="flex flex-col">
            <span className="text-xs font-semibold text-foreground">
              {action === "encrypt"
                ? APP_CONFIG.status.ENCRYPTING_STREAM
                : APP_CONFIG.status.DECRYPTING_STREAM}
            </span>
            <span className="text-[11px] text-muted-foreground">{message}</span>
          </div>
        </div>
      )}

      {/* Error State */}
      {type === "error" && (
        <Alert
          variant="destructive"
          className="border-destructive/30 bg-destructive/10 text-destructive shadow-xs"
        >
          <AlertTriangle className="size-4" />
          <AlertTitle className="text-xs font-semibold">Notice</AlertTitle>
          <AlertDescription className="text-xs font-mono pt-0.5">
            {message}
          </AlertDescription>
        </Alert>
      )}
    </div>
  );
};
