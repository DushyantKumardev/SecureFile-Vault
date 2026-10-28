import React, { useState } from "react";
import { KeyRound, Eye, EyeOff, Zap, Lock, Unlock } from "lucide-react";
import { Card, CardContent } from "@/components/ui/card";
import { Input } from "@/components/ui/input";
import { Button } from "@/components/ui/button";
import { Progress } from "@/components/ui/progress";
import { computePasswordEntropy, generateCryptographicKey } from "@/lib/bridge";
import type { VaultAction } from "@/types/vault";

interface CipherFormProps {
  password: string;
  onPasswordChange: (pwd: string) => void;
  onExecute: (action: VaultAction) => void;
  isLoading: boolean;
  hasFile: boolean;
  isEncryptedFile: boolean;
}

export const CipherForm: React.FC<CipherFormProps> = ({
  password,
  onPasswordChange,
  onExecute,
  isLoading,
  hasFile,
  isEncryptedFile,
}) => {
  const [showPassword, setShowPassword] = useState(false);
  const entropy = computePasswordEntropy(password);

  const handleGenerate = () => {
    const key = generateCryptographicKey();
    onPasswordChange(key);
    setShowPassword(true);
  };

  const getStrengthColor = (score: number) => {
    switch (score) {
      case 4:
        return "text-emerald-600 dark:text-emerald-400";
      case 3:
        return "text-teal-600 dark:text-teal-400";
      case 2:
        return "text-amber-600 dark:text-amber-400";
      default:
        return "text-destructive";
    }
  };

  const getProgressClass = (score: number) => {
    switch (score) {
      case 4:
        return "[&>div]:bg-emerald-500";
      case 3:
        return "[&>div]:bg-teal-500";
      case 2:
        return "[&>div]:bg-amber-500";
      default:
        return "[&>div]:bg-destructive";
    }
  };

  return (
    <Card className="border-border/80 bg-linear-to-b from-card via-card to-card/90 shadow-sm backdrop-blur-xs relative overflow-hidden">
      {/* Top subtle gradient highlight bar */}
      <div className="absolute top-0 inset-x-0 h-0.5 bg-linear-to-r from-transparent via-primary/50 to-transparent opacity-80" />

      <CardContent className="p-5 space-y-5">
        {/* Header */}
        <div className="flex items-center justify-between">
          <h3 className="text-sm font-semibold tracking-tight text-foreground flex items-center gap-2">
            <KeyRound className="size-4 text-primary" />
            Security Key
          </h3>
          <Button
            variant="ghost"
            size="sm"
            type="button"
            onClick={handleGenerate}
            disabled={isLoading}
            className="text-xs text-primary hover:text-primary hover:bg-primary/10 gap-1.5 h-7 px-2 font-medium transition-all"
          >
            <Zap className="size-3.5" />
            Generate Key
          </Button>
        </div>

        {/* Password Input Group */}
        <div className="space-y-2">
          <div className="relative flex items-center group">
            <Input
              id="cipher-key-input"
              type={showPassword ? "text" : "password"}
              value={password}
              onChange={(e) => onPasswordChange(e.target.value)}
              placeholder="Enter cryptographic security key..."
              disabled={isLoading}
              className="pr-20 h-10 text-xs font-mono bg-muted/30 border-border/80 focus-visible:ring-1 focus-visible:ring-primary/40 focus-visible:border-primary/50 transition-all shadow-inner"
            />
            <Button
              type="button"
              variant="ghost"
              size="sm"
              onClick={() => setShowPassword((prev) => !prev)}
              disabled={isLoading}
              className="absolute right-1 size-8 p-0 text-muted-foreground hover:text-foreground transition-colors"
              title={showPassword ? "Hide Key" : "Reveal Key"}
            >
              {showPassword ? (
                <EyeOff className="size-4" />
              ) : (
                <Eye className="size-4" />
              )}
            </Button>
          </div>

          {/* Password Strength Indicator */}
          {password && (
            <div className="px-3 py-2 rounded-lg bg-muted/20 border border-border/60 space-y-1.5 transition-all">
              <div className="flex items-center justify-between text-[11px] font-medium">
                <span className="text-muted-foreground">Quality:</span>
                <span
                  className={`font-semibold ${getStrengthColor(entropy.score)}`}
                >
                  {entropy.label}
                </span>
              </div>
              <Progress
                value={entropy.percent}
                className={`h-1 bg-muted ${getProgressClass(entropy.score)}`}
              />
            </div>
          )}
        </div>

        {/* Action Buttons */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-3 pt-1">
          <Button
            type="button"
            variant="default"
            size="lg"
            onClick={() => onExecute("encrypt")}
            disabled={!hasFile || !password || isLoading}
            className="w-full gap-2 h-11 text-xs font-semibold shadow-xs active:scale-[0.99] transition-all disabled:opacity-50"
          >
            <Lock className="size-4" />
            <span>Encrypt In-Place</span>
          </Button>

          <Button
            type="button"
            variant="outline"
            size="lg"
            onClick={() => onExecute("decrypt")}
            disabled={!hasFile || !password || isLoading}
            className={`w-full gap-2 h-11 text-xs font-semibold transition-all active:scale-[0.99] disabled:opacity-50 ${
              isEncryptedFile
                ? "border-emerald-500/40 text-emerald-600 dark:text-emerald-400 bg-emerald-500/10 hover:bg-emerald-500/20"
                : "hover:bg-muted/70"
            }`}
          >
            <Unlock className="size-4" />
            <span>Decrypt &amp; Restore</span>
          </Button>
        </div>
      </CardContent>
    </Card>
  );
};
