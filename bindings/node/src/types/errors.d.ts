/**
 * Native failure identifiers. NOT_FOUND denotes invalid cursor access; missing
 * gets return null. INVALID_ARGUMENT covers native parameters, lengths and
 * foreign snapshots. IO_ERROR preserves unclassified filesystem errors;
 * CORRUPTION is engine-detected. DATABASE_LOCKED identifies directory contention;
 * DATABASE_CLOSED indicates a closed resource. UNSUPPORTED denotes unavailable
 * features, and INTERNAL_ERROR an unexpected exception or unknown native status.
 */
export type ErrorCode = 'NOT_FOUND' | 'INVALID_ARGUMENT' | 'IO_ERROR' | 'CORRUPTION' |
    'DATABASE_LOCKED' | 'DATABASE_CLOSED' | 'UNSUPPORTED' | 'INTERNAL_ERROR';

/** Standard Error rejected by a native worker, with its original message. */
export interface DbError extends Error {
    /** Machine-readable category; do not classify failures by message text. */
    code: ErrorCode;
}
