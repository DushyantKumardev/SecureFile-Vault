import React from "react";
import { History, Trash2, CheckCircle2 } from "lucide-react";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
  CardDescription,
} from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import type { AuditRecord } from "@/types/vault";

interface AuditLogsProps {
  logs: AuditRecord[];
  onClear: () => void;
}

export const AuditLogs: React.FC<AuditLogsProps> = ({ logs, onClear }) => {
  return (
    <Card className="border-border/80 bg-linear-to-b from-card via-card to-card/90 shadow-sm backdrop-blur-xs relative overflow-hidden">
      {/* Top subtle gradient highlight bar */}
      <div className="absolute top-0 inset-x-0 h-0.5 bg-linear-to-r from-transparent via-primary/50 to-transparent opacity-80" />

      <CardHeader className="p-5 pb-3 flex flex-row items-center justify-between">
        <div className="space-y-0.5">
          <CardTitle className="text-base font-semibold tracking-tight text-foreground flex items-center gap-2">
            <div className="p-1 rounded-md bg-primary/10 text-primary">
              <History className="size-3.5" />
            </div>
            Security Audit Trail
          </CardTitle>
          <CardDescription className="text-xs text-muted-foreground">
            Immutable log of all encryption and decryption operations performed
            in this session.
          </CardDescription>
        </div>
        {logs.length > 0 && (
          <Button
            variant="ghost"
            size="sm"
            onClick={onClear}
            className="text-xs text-destructive hover:text-destructive hover:bg-destructive/10 h-8 gap-1.5 transition-colors"
          >
            <Trash2 className="size-3.5" />
            Clear Trail
          </Button>
        )}
      </CardHeader>

      <CardContent className="p-5 pt-2">
        {logs.length === 0 ? (
          <div className="text-center py-12 space-y-2 border border-dashed rounded-xl bg-muted/10">
            <div className="size-10 rounded-full bg-muted mx-auto flex items-center justify-center text-muted-foreground">
              <History className="size-5" />
            </div>
            <p className="text-xs font-medium text-foreground">
              No cryptographic operations logged yet
            </p>
            <p className="text-[11px] text-muted-foreground">
              Operations will be logged automatically with timestamps upon
              execution.
            </p>
          </div>
        ) : (
          <div className="divide-y divide-border border rounded-xl overflow-hidden bg-card">
            {logs.map((record) => (
              <div
                key={record.id}
                className="p-3.5 flex items-center justify-between gap-3 text-xs hover:bg-muted/30 transition-colors"
              >
                <div className="flex items-center gap-3 min-w-0">
                  <Badge
                    variant="outline"
                    className={`font-mono text-[10px] uppercase font-bold shrink-0 ${
                      record.action === "ENCRYPT"
                        ? "border-blue-500/30 text-blue-600 dark:text-blue-400 bg-blue-500/10"
                        : "border-emerald-500/30 text-emerald-600 dark:text-emerald-400 bg-emerald-500/10"
                    }`}
                  >
                    {record.action}
                  </Badge>

                  <div className="flex flex-col min-w-0">
                    <span
                      className="font-semibold text-foreground truncate"
                      title={record.fullPath}
                    >
                      {record.fileName}
                    </span>
                    <span className="text-[10px] text-muted-foreground truncate font-mono">
                      {record.fullPath}
                    </span>
                  </div>
                </div>

                <div className="flex items-center gap-3 shrink-0">
                  <div className="flex flex-col items-end">
                    <span className="font-mono text-[11px] text-foreground font-medium">
                      {record.timestamp}
                    </span>
                    <span className="text-[10px] text-muted-foreground">
                      {record.date}
                    </span>
                  </div>
                  <CheckCircle2 className="size-4 text-emerald-500" />
                </div>
              </div>
            ))}
          </div>
        )}
      </CardContent>
    </Card>
  );
};
