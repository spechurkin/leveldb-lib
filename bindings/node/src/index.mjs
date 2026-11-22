/** ES module facade exporting the same documented classes and resource contracts
 * as index.cjs; no separate engine, wrappers or implicit conversions are added.
 * @module multileveldb/esm
 */
import api from './index.cjs';

export const {Database, Batch, Snapshot, Cursor} = api;
export default api;
