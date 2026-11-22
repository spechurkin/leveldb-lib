/// <reference types="node" />
/**
 * Binary-safe Node-API library. Native worker errors reject with DbError;
 * module/native-loader failures occur before calls and use ordinary loader Errors.
 * Use the matching prepared addon/shared library and await resource cleanup.
 */

export {Database};

export {Batch};

export {Snapshot};

export {Cursor};
export type {OpenOptions, ReadOptions, WriteOptions} from './types/options';
export type {ErrorCode, DbError} from './types/errors';

/** Default ESM/CommonJS namespace, containing the same four named classes. */
declare const api: { Database: typeof Database; Batch: typeof Batch; Snapshot: typeof Snapshot; Cursor: typeof Cursor };
export default api;
