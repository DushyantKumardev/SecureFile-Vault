import React from 'react';
import { Sun, Moon, Laptop } from 'lucide-react';
import { useTheme } from '@/components/theme-provider';

interface ThemeToggleProps {
  className?: string;
}

export const ThemeToggle: React.FC<ThemeToggleProps> = ({ className = '' }) => {
  const { theme, setTheme } = useTheme();

  return (
    <div
      className={`p-1 rounded-lg border border-border bg-muted/40 flex items-center justify-between text-xs select-none ${className}`}
    >
      <button
        type="button"
        onClick={() => setTheme('light')}
        className={`flex-1 flex items-center justify-center gap-1.5 py-1 px-2 rounded-md transition-all text-[11px] font-medium ${
          theme === 'light'
            ? 'bg-background text-foreground shadow-xs font-semibold'
            : 'text-muted-foreground hover:text-foreground'
        }`}
        title="Light Mode"
      >
        <Sun className="size-3.5" />
        <span>Light</span>
      </button>

      <button
        type="button"
        onClick={() => setTheme('dark')}
        className={`flex-1 flex items-center justify-center gap-1.5 py-1 px-2 rounded-md transition-all text-[11px] font-medium ${
          theme === 'dark'
            ? 'bg-background text-foreground shadow-xs font-semibold'
            : 'text-muted-foreground hover:text-foreground'
        }`}
        title="Dark Mode"
      >
        <Moon className="size-3.5" />
        <span>Dark</span>
      </button>

      <button
        type="button"
        onClick={() => setTheme('system')}
        className={`flex-1 flex items-center justify-center gap-1.5 py-1 px-2 rounded-md transition-all text-[11px] font-medium ${
          theme === 'system'
            ? 'bg-background text-foreground shadow-xs font-semibold'
            : 'text-muted-foreground hover:text-foreground'
        }`}
        title="System Preference"
      >
        <Laptop className="size-3.5" />
        <span>System</span>
      </button>
    </div>
  );
};
