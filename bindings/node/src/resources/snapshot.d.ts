import {Resource} from '../internal/resource';

/**
 * Immutable engine read view created by Database.createSnapshot. Pass to get or
 * iterator on its originating DB. Inherited close releases this reference;
 * existing snapshot cursors retain theirs. Held snapshots may retain disk data.
 */
export class Snapshot extends Resource {
}
