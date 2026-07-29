# ATPK Binary Format

ATPK is a little-endian sectioned container for directed asset graphs. Every
file begins with a fixed 32-byte header followed by a fixed-width directory and
five standard payload sections.

## Header

| Offset | Width | Field |
|---:|---:|---|
| 0 | 4 | Magic `ATPK` encoded as `0x4b505441` |
| 4 | 2 | Format version |
| 6 | 2 | File flags |
| 8 | 4 | Directory entry count |
| 12 | 8 | Directory byte offset |
| 20 | 8 | Declared total file size |
| 28 | 4 | Checksum of all bytes after the header |

The current version is 1. The declared size must equal the exact supplied byte
length. Counts and offsets are rejected before allocation when they exceed the
library limits or cannot fit in the input.

## Directory entries

Each 32-byte entry contains a 32-bit type, 32-bit flags field, 64-bit payload
offset, 64-bit payload size, and 64-bit auxiliary value. Section ranges must be
inside the file and must not overlap after ordering by offset.

Standard types are:

| Type | Section | Entry layout |
|---:|---|---|
| 1 | Strings | count, then repeated length and UTF-8 bytes |
| 2 | Nodes | count, then ID, name index, first attribute, attribute count |
| 3 | Edges | count, then source ID, target ID, label index |
| 4 | Attributes | count, then node ID, key index, value index |
| 5 | Metadata | count, then key index, value index |

Strings and nodes are mandatory. Edges, attributes, and metadata may be empty.
Unknown directory entries are accepted only when the caller enables them. Their
directory metadata remains inspectable through `atlas_document_section_at`, but
the payload is not interpreted or retained as semantic document state.
`atlas_document_serialize` therefore emits a canonical package containing only
the five standard section types; accepted unknown sections are intentionally
dropped during semantic serialization. Applications that require byte-preserving
container rewriting must retain and rewrite the original package bytes outside
the semantic document API.

## Semantic invariants

Node IDs are unique. Every string reference must resolve inside the string
table. Every edge endpoint and attribute owner must resolve to a node. Node
attribute slices must describe the exact contiguous attribute ownership encoded
in the attribute section. Metadata keys are unique by default, with an explicit
parse option for applications that accept duplicates.

After decoding, names may be normalized, ID and canonical-name indexes are
built, edge endpoints are resolved to stable node indexes, and outgoing and
incoming edge-order arrays are constructed. Published documents are immutable.
