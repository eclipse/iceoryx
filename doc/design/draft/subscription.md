# Subscription

## Summary

In this document we analyse how many and what kind of chunks will be provided
to a subscriber on subscription. The answer depends on the number of publishers,
how many samples were send in the past and how large the requested _HistorySize_
is.

The goal is to craft and implement a design which is unable to deadlock,
massively slow down (caused by deadlock discovery) or crash RouDi.

## Terminology

- **ChunkHistory** the last N samples which the publishers of a service have
   published
- **HistorySize** the number of samples the subscriber will receive at most on
   subscription
- **DeliveryTime** the point in time where the publisher writes the samples in
   the _ChunkHistory_. This is the first task right before the actual delivery to
   the subscribers starts.

## Requirements

### General

- RouDi is allowed to block any arbitrary user process.
- RouDi is allowed to block threads owned by RouDi.
- No process is allowed to block any thread in RouDi via logic or lock.

Since RouDi is our central daemon and the whole system is corrupted when it fails
we can assume for now that RouDi with its inner logic never fails. But we have to
ensure that RouDi is not corrupted by any external instance like a publisher
application which holds a lock and dies which would lead to a deadlock in RouDi.

### Subscribing without concurrent send

Let's assume a subscriber S would like to subscribe to a service which has two
publishers P1 and P2. Furthermore, we assume that P1 and P2 have send the
following samples:

| Publisher | Timepoint | Sample | | Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|-|:---------:|:---------:|:------:|
| P1        | 1         | A      | | P2        | 20        | F      |
| P1        | 11        | B      | | P2        | 22        | G      |
| P1        | 21        | C      | | P2        | 24        | H      |
| P1        | 31        | D      | | P2        | 40        | I      |
| P1        | 41        | E      | | P2        | 42        | J      |

Let's assume the subscriber subscribes to the service.

#### Subscription with _HistorySize_ 0

The subscriber will not be provided with samples from the history.

#### Subscription with _HistorySize_ 1

The subscriber cache contains the one sample ordered by _DeliveryTime_.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P2        | 42        | J      |

#### Subscription with _HistorySize_ 6

The subscriber cache contains the six samples ordered by _DeliveryTime_.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P2        | 42        | J      |
| P1        | 41        | E      |
| P2        | 40        | I      |
| P1        | 31        | D      |
| P2        | 24        | H      |
| P2        | 22        | G      |

#### Subscription with _HistorySize_ 20

The subscriber cache contains the ten samples ordered after _DeliveryTime_.
It contains only ten since there are no more samples available on publisher side.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P2        | 42        | J      |
| P1        | 41        | E      |
| P2        | 40        | I      |
| P1        | 31        | D      |
| P2        | 24        | H      |
| P2        | 22        | G      |
| P1        | 21        | C      |
| P2        | 20        | F      |
| P1        | 11        | B      |
| P1        | 1         | A      |

### Subscribing with concurrent send

Let's assume publisher P1 would like to deliver the sample K at timepoint
52 while the subscriber requested subsription. Furthermore, the subscriber
requests always a _HistorySize_ of 2.

All the listed cases should behave identical if there is more then one
concurrent publish in progress.

#### Publisher before _DeliveryTime_

The subscriber cache contains two samples.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P2        | 42        | J      |
| P1        | 41        | E      |

The sample K will be delivered after successful subscription.

#### Publisher after _DeliveryTime_

The subscriber cache contains two samples.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P1        | 51        | K      |
| P2        | 42        | J      |

The sample K will not be delivered by the publisher since the _DeliveryTime_
is defined as exactly the timepoint after the sample is placed into the ChunkHistory
and the subscriber cache is based on that ChunkHistory.

### Subscribing After Publisher Stops Offering

In this case we assume that publisher P1 has stopped offering the service
before the subscription was requested.

#### Subscription with _HistorySize_ 4

The subscriber cache contains the four samples ordered after _DeliveryTime_.

| Publisher | Timepoint | Sample |
|:---------:|:---------:|:------:|
| P2        | 42        | J      |
| P1        | 41        | E      |
| P2        | 40        | I      |
| P1        | 31        | D      |



## Design

### Idea 1: One Ring Buffer Which Contains The Samples

- All publishers are sharing one random access lock-free ring-buffer.
- The ring-buffer is filled with shared-chunks which can be copied in a threadsafe
    and lock-free manner.
- A publisher delivers a sample by writing that sample into the shared ring-buffer.
- Every subscriber owns an index to their own read position of the ring-buffer.
- On subscription the subscriber will acquire a const reference to the ring-buffer.
- When subscriber acquires a sample with take, the shared-chunk is copied from
    the index position and the index is incremented.
- When the publisher delivers a sample it will verify all subscriber indices.
   If a subscriber `index > ring-buffer-head - subscriberHistorySize` the index
   is incremented.

#### Open Points

- Many chunks will not be released even when no susbcriber is using them.
  - This might be solvable by having the ring-buffer size as the maximum of
    history size and subscriber queue capacity?!
- ring-buffer size? Since the ring-buffer is shared between multiple publishers
  the size has to be defined via some service subscription options.
- Who owns the ring-buffer? Since it is shared between multiple publishers and
  a subscriber should be able to read samples after all publisher have stopped
  offering the service, the ring-buffer should be owned by the service?

### Considerations

### Solution

#### Pitfalls & Testing

## Open Issues

