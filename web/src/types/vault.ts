export type VaultAction = 'encrypt' | 'decrypt';

export type StatusType = 'idle' | 'loading' | 'success' | 'error';

export interface OperationStatus {
  type: StatusType;
  message: string;
  loading: boolean;
  timestamp?: string;
  action?: VaultAction;
}

export interface FileMetadata {
  rawPath: string;
  fileName: string;
  extension: string;
  isEncrypted: boolean;
  directory?: string;
}

export interface AuditRecord {
  id: string;
  action: 'ENCRYPT' | 'DECRYPT';
  fileName: string;
  fullPath: string;
  timestamp: string;
  date: string;
  success: boolean;
  notes?: string;
}

export type ActiveTab = 'vault' | 'history';

export interface PasswordEntropy {
  score: number;
  label: 'None' | 'Weak' | 'Fair' | 'Strong' | 'Very Strong';
  percent: number;
  checks: {
    length: boolean;
    number: boolean;
    upper: boolean;
    symbol: boolean;
  };
}
