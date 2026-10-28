import React from "react";
import {
  ShieldCheck,
  History,
  Lock,
  ChevronLeft,
  ChevronRight,
  type LucideIcon,
} from "lucide-react";
import {
  Sidebar as ShadcnSidebar,
  SidebarContent,
  SidebarFooter,
  SidebarGroup,
  SidebarGroupContent,
  SidebarHeader,
  SidebarMenu,
  SidebarMenuBadge,
  SidebarMenuButton,
  SidebarMenuItem,
  SidebarRail,
  SidebarSeparator,
  SidebarTrigger,
} from "@/components/ui/sidebar";
import { ThemeToggle } from "@/components/ThemeToggle";
import { APP_CONFIG } from "@/constants";
import type { ActiveTab } from "@/types/vault";

interface SidebarProps {
  activeTab: ActiveTab;
  onTabChange: (tab: ActiveTab) => void;
  auditCount: number;
  engineActive: boolean;
}

interface Tabs {
  name: string;
  icon: LucideIcon;
  type: ActiveTab;
  count?: number;
}

export const Sidebar: React.FC<SidebarProps> = ({
  activeTab,
  onTabChange,
  auditCount,
}) => {
  const Tabs: Tabs[] = [
    {
      name: APP_CONFIG.tabs.VAULT.name,
      icon: ShieldCheck,
      type: APP_CONFIG.tabs.VAULT.id,
    },
    {
      name: APP_CONFIG.tabs.HISTORY.name,
      icon: History,
      type: APP_CONFIG.tabs.HISTORY.id,
      count: auditCount,
    },
  ];

  return (
    <ShadcnSidebar
      collapsible="icon"
      className="border-r border-sidebar-border bg-sidebar select-none relative"
    >
      {/* Trigger button anchored adjacent to the sidebar rail */}
      <div className="absolute top-3.5 -right-3 z-50 flex items-center justify-center">
        <SidebarTrigger className="size-6 rounded-full border border-sidebar-border bg-card shadow-xs text-sidebar-foreground/80 hover:text-sidebar-foreground hover:bg-sidebar-accent transition-all flex items-center justify-center cursor-pointer p-0">
          <ChevronRight className="size-3.5 group-data-[state=expanded]:hidden" />
          <ChevronLeft className="size-3.5 group-data-[state=collapsed]:hidden" />
        </SidebarTrigger>
      </div>
      <SidebarHeader className="p-3 group-data-[collapsible=icon]:p-2 flex items-center justify-center">
        {/* App Branding */}
        <div className="flex items-center gap-2.5 px-1 py-1 group-data-[collapsible=icon]:px-0 group-data-[collapsible=icon]:gap-0 group-data-[collapsible=icon]:justify-center w-full overflow-hidden">
          <div className="size-8 shrink-0 rounded-lg bg-primary flex items-center justify-center text-primary-foreground shadow-xs">
            <Lock className="size-4" />
          </div>
          <div className="flex flex-col min-w-0 group-data-[collapsible=icon]:hidden">
            <span className="font-semibold text-sm tracking-tight text-sidebar-foreground truncate">
              {APP_CONFIG.name}
            </span>
            <span className="text-[10px] text-muted-foreground font-mono">
              v{APP_CONFIG.version}
            </span>
          </div>
        </div>
      </SidebarHeader>

      <SidebarSeparator className="my-1 opacity-70" />

      <SidebarContent className="px-2 py-1 group-data-[collapsible=icon]:p-0 group-data-[collapsible=icon]:overflow-hidden">
        <SidebarGroup className="p-1 group-data-[collapsible=icon]:p-1.5 group-data-[collapsible=icon]:items-center">
          <SidebarGroupContent>
            <SidebarMenu className="gap-1 group-data-[collapsible=icon]:items-center">
              {Tabs.map((tab) => (
                <SidebarMenuItem className="group-data-[collapsible=icon]:flex group-data-[collapsible=icon]:justify-center">
                  <SidebarMenuButton
                    isActive={activeTab === tab.type}
                    onClick={() => onTabChange(tab.type)}
                    tooltip={tab.name}
                    className={`group-data-[collapsible=icon]:justify-center group-data-[collapsible=icon]:p-0! ${
                      activeTab === tab.type
                        ? "bg-primary text-primary-foreground hover:bg-primary/90 hover:text-primary-foreground font-semibold shadow-xs"
                        : "text-sidebar-foreground/80 hover:bg-sidebar-accent hover:text-sidebar-accent-foreground font-medium"
                    }`}
                  >
                    <tab.icon className="size-4 shrink-0" />
                    <span className="group-data-[collapsible=icon]:hidden">
                      {tab.name}
                    </span>
                    {tab.count && tab.count > 0 && (
                      <SidebarMenuBadge
                        className={
                          activeTab === tab.type
                            ? "bg-sidebar-accent text-sidebar-accent-foreground border-transparent"
                            : "bg-muted text-muted-foreground"
                        }
                      >
                        {tab.count}
                      </SidebarMenuBadge>
                    )}
                  </SidebarMenuButton>
                </SidebarMenuItem>
              ))}
            </SidebarMenu>
          </SidebarGroupContent>
        </SidebarGroup>
      </SidebarContent>

      <SidebarFooter className="p-3 group-data-[collapsible=icon]:hidden">
        <ThemeToggle />
      </SidebarFooter>
      <SidebarRail />
    </ShadcnSidebar>
  );
};

export default Sidebar;
