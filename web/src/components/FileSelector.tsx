import React, { useState } from "react";
import { FileText, CheckCircle2, X, UploadCloud } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";
import { Button } from "@/components/ui/button";
import { Badge } from "@/components/ui/badge";
import type { FileMetadata } from "@/types/vault";

interface FileSelectorProps {
  file: FileMetadata | null;
  onBrowse: () => void;
  onClear: () => void;
  disabled?: boolean;
}

export const FileSelector: React.FC<FileSelectorProps> = ({
  file,
  onBrowse,
  onClear,
  disabled = false,
}) => {
  const [isDragOver, setIsDragOver] = useState(false);

  const getExtensionColor = (ext: string, isEnc: boolean) => {
    if (isEnc)
      return "bg-emerald-500/10 text-emerald-600 dark:text-emerald-400 border-emerald-500/30";
    switch (ext) {
      case "PDF":
        return "bg-red-500/10 text-red-600 dark:text-red-400 border-red-500/30";
      case "PNG":
      case "JPG":
      case "JPEG":
        return "bg-blue-500/10 text-blue-600 dark:text-blue-400 border-blue-500/30";
      case "ZIP":
      case "RAR":
      case "7Z":
        return "bg-amber-500/10 text-amber-600 dark:text-amber-400 border-amber-500/30";
      case "DOC":
      case "DOCX":
        return "bg-indigo-500/10 text-indigo-600 dark:text-indigo-400 border-indigo-500/30";
      default:
        return "bg-muted text-muted-foreground border-border";
    }
  };

  return (
    <Card className="border-border/80 bg-linear-to-b from-card via-card to-card/90 shadow-sm backdrop-blur-xs relative overflow-hidden">
      {/* Top subtle gradient highlight bar */}
      <div className="absolute top-0 inset-x-0 h-0.5 bg-linear-to-r from-transparent via-primary/50 to-transparent opacity-80" />

      <CardContent className="p-5 space-y-4">
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold tracking-tight text-foreground flex items-center gap-2">
            <FileText className="size-4 text-primary" />
            Target File
          </h3>
          {file?.isEncrypted ? (
            <Badge
              variant="outline"
              className="border-emerald-500/40 text-emerald-600 dark:text-emerald-400 bg-emerald-500/10 text-[11px] gap-1 font-medium shadow-xs"
            >
              <CheckCircle2 className="size-3" />
              Encrypted
            </Badge>
          ) : file ? (
            <Badge
              variant="outline"
              className="text-[11px] font-medium border-border/80 bg-muted/30"
            >
              Plain File
            </Badge>
          ) : null}
        </div>

        {/* Selected File Card or Empty Picker */}
        {file ? (
          <div className="p-3.5 rounded-xl border border-border/90 bg-muted/20 flex items-center justify-between gap-3 shadow-xs transition-all">
            <div className="flex items-center gap-3 overflow-hidden min-w-0">
              <Badge
                variant="outline"
                className={`font-mono text-xs px-2 py-1 font-bold shrink-0 ${getExtensionColor(
                  file.extension,
                  file.isEncrypted,
                )}`}
              >
                .{file.extension}
              </Badge>
              <div className="flex flex-col min-w-0">
                <span
                  className="text-sm font-medium text-foreground truncate"
                  title={file.rawPath}
                >
                  {file.fileName}
                </span>
                <span className="text-[11px] text-muted-foreground truncate font-mono">
                  {file.rawPath}
                </span>
              </div>
            </div>

            <div className="flex items-center gap-1.5 shrink-0">
              <Button
                variant="outline"
                size="sm"
                onClick={onBrowse}
                disabled={disabled}
                className="text-xs h-7 px-2.5 font-medium"
              >
                Change
              </Button>
              <Button
                variant="ghost"
                size="icon"
                onClick={onClear}
                disabled={disabled}
                className="size-7 text-muted-foreground hover:text-destructive transition-colors"
                title="Clear selection"
              >
                <X className="size-3.5" />
              </Button>
            </div>
          </div>
        ) : (
          <div
            onDragOver={(e) => {
              e.preventDefault();
              setIsDragOver(true);
            }}
            onDragLeave={() => setIsDragOver(false)}
            onDrop={(e) => {
              e.preventDefault();
              setIsDragOver(false);
              onBrowse();
            }}
            onClick={onBrowse}
            className={`border border-dashed rounded-xl p-5 text-center cursor-pointer transition-all duration-150 flex flex-col items-center justify-center gap-2 group ${
              isDragOver
                ? "border-primary bg-primary/5"
                : "border-border hover:border-primary/60 hover:bg-muted/20"
            }`}
          >
            <UploadCloud className="size-6 text-muted-foreground group-hover:text-primary transition-colors" />
            <p className="text-xs text-muted-foreground group-hover:text-foreground transition-colors">
              Drag &amp; drop a file here, or{" "}
              <span className="text-primary font-medium underline underline-offset-2">
                browse
              </span>
            </p>
          </div>
        )}
      </CardContent>
    </Card>
  );
};
