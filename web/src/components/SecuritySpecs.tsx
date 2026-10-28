import React from "react";
import {
  Cpu,
  ShieldCheck,
  Binary,
  Database,
  Lock,
  CheckCircle2,
} from "lucide-react";
import {
  Card,
  CardContent,
  CardHeader,
  CardTitle,
  CardDescription,
} from "@/components/ui/card";
import { Badge } from "@/components/ui/badge";
import { Separator } from "@/components/ui/separator";

export const SecuritySpecs: React.FC = () => {
  return (
    <div className="space-y-4">
      <Card className="border-border/80 bg-linear-to-b from-card via-card to-card/90 shadow-sm backdrop-blur-xs relative overflow-hidden">
        {/* Top subtle gradient highlight bar */}
        <div className="absolute top-0 inset-x-0 h-0.5 bg-linear-to-r from-transparent via-primary/50 to-transparent opacity-80" />

        <CardHeader className="p-5 pb-3">
          <CardTitle className="text-base font-semibold tracking-tight text-foreground flex items-center gap-2">
            <div className="p-1 rounded-md bg-primary/10 text-primary">
              <Cpu className="size-3.5" />
            </div>
            Cryptographic Engine Specifications
          </CardTitle>
          <CardDescription className="text-xs text-muted-foreground">
            Architecture and algorithmic implementation details for academic
            evaluation.
          </CardDescription>
        </CardHeader>

        <CardContent className="p-5 pt-2 space-y-4">
          <div className="grid grid-cols-1 md:grid-cols-2 gap-3 text-xs">
            <div className="p-3.5 rounded-xl border border-border bg-muted/20 space-y-1.5">
              <div className="flex items-center justify-between">
                <span className="font-semibold text-foreground flex items-center gap-1.5">
                  <Binary className="size-3.5 text-primary" />
                  Primary Cipher
                </span>
                <Badge variant="outline" className="font-mono text-[10px]">
                  Bitwise XOR
                </Badge>
              </div>
              <p className="text-[11px] text-muted-foreground leading-relaxed">
                Bitwise symmetric stream cipher transforming source bytes in
                place using dynamically generated pseudo-random keystreams.
              </p>
            </div>

            <div className="p-3.5 rounded-xl border border-border bg-muted/20 space-y-1.5">
              <div className="flex items-center justify-between">
                <span className="font-semibold text-foreground flex items-center gap-1.5">
                  <Lock className="size-3.5 text-primary" />
                  Key Derivation
                </span>
                <Badge variant="outline" className="font-mono text-[10px]">
                  FNV-1a Chain
                </Badge>
              </div>
              <p className="text-[11px] text-muted-foreground leading-relaxed">
                32-bit state multiplication with 16777619 prime factor and
                offset mixing to avoid fixed repetitive period XOR attacks.
              </p>
            </div>

            <div className="p-3.5 rounded-xl border border-border bg-muted/20 space-y-1.5">
              <div className="flex items-center justify-between">
                <span className="font-semibold text-foreground flex items-center gap-1.5">
                  <Database className="size-3.5 text-primary" />
                  Buffer Size
                </span>
                <Badge variant="outline" className="font-mono text-[10px]">
                  8192 Bytes
                </Badge>
              </div>
              <p className="text-[11px] text-muted-foreground leading-relaxed">
                Large chunk streaming avoids loading entire multi-gigabyte files
                into RAM, guaranteeing stable memory footprint on low-spec
                hardware.
              </p>
            </div>

            <div className="p-3.5 rounded-xl border border-border bg-muted/20 space-y-1.5">
              <div className="flex items-center justify-between">
                <span className="font-semibold text-foreground flex items-center gap-1.5">
                  <ShieldCheck className="size-3.5 text-primary" />
                  Fail-Safe Recovery
                </span>
                <Badge variant="outline" className="font-mono text-[10px]">
                  Auto Backup
                </Badge>
              </div>
              <p className="text-[11px] text-muted-foreground leading-relaxed">
                Automated pre-cipher snapshots ensure complete data preservation
                and instant recovery if an operation is interrupted.
              </p>
            </div>
          </div>

          <Separator />

          <div className="p-4 rounded-xl border border-border bg-muted/10 space-y-2">
            <h4 className="text-xs font-semibold text-foreground flex items-center gap-2">
              <CheckCircle2 className="size-3.5 text-emerald-500" />
              Runtime Layer Separation
            </h4>
            <div className="text-[11px] text-muted-foreground font-mono space-y-1">
              <div>
                Frontend: React 19 + TypeScript + Tailwind CSS v4 + shadcn/ui
              </div>
              <div>
                Bridge: Microsoft Edge Chromium WebView2 IPC (JSON Message
                Channel)
              </div>
              <div>Core Backend: Pure C11 (GCC) &amp; Modern C++17 (G++)</div>
            </div>
          </div>
        </CardContent>
      </Card>
    </div>
  );
};
