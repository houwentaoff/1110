#ifndef VIDEOFMT_MP4R_H
#define VIDEOFMT_MP4R_H
/*===========================================================================

            V I D E O   F O R M A T S   -   M P 4   R E A D E R
                  I N T E R N A L   H E A D E R   F I L E

DESCRIPTION
  This header file contains all the internal definitions to the MP4 file
  format reader portion of the video formats services.

Copyright(c) 2002-2003 by QUALCOMM, Incorporated. All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to this file.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/multimedia/mmservices/mpeg4fileformat/parser/main/latest/inc/videofmt_mp4r.h#5 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
02/12/06   bdv     Added ability to parse avcc atom irrespective of its size
12/09/05   rpw     Added AMR-WB and AMR-WB+ support
09/16/03   rpw     Added video_fmt_mp4r_read_network_word function and
                   bs_mp3_id_bit field to video_fmt_mp4r_context_type.
09/05/03   enj     Removing FEATURE_VIDEOFMT (i.e. permanently enabling it)
07/11/03   rpw     Added FEATURE_AAC_ONLY_EXTRA_RESYNC_MARKERS code to insert
                   resync markers at the beginning of each AAC frame, for
                   AAC bitstreams with audio_object_type between 17 and 27,
                   inclusive.
06/25/03   rpw     Created file, from original videofmt_mp4i.h file.  Renamed
                   all symbols from video_fmt_mp4_* to video_fmt_mp4r_*.
06/23/03   rpw     Replaced FEATURE_MP4_DECODER with FEATURE_VIDEOFMT.
06/03/03   rpw     Added code to recognize MPEG-4 video short header format
                   as H.263 content.  Added type for MPEG-4 AAC audio.
05/23/03   rpw     Added timed text atom minimal support.
05/22/03   rpw     Added interface for finding nearest sync sample at or
                   after any given sample in a stream.
                   Added stream byte offset to sample information.
05/22/03   rpw     Fixed typo in 3GPP2 EVRC sample entry atom names.
05/12/03    ny     Definition of data types to support the structure-oriented
                   atom definitions. Added video_fmt_atom_field_type and
                   video_fmt_consume_atom_structure_type, as well as new
                   function prototypes.
03/18/03   rpw     Merged in changes from MPEG4_FILE_FORMAT branch.
03/17/03   rpw     Fixed stream reading bugs when read unit is set
                   to VIDEO_FMT_DATA_UNIT_FRAME.
02/25/03   rpw     Added method of getting sample information (timestamps,
                   sample sizes, sync samples, etc.)
02/24/03   rpw     Replaced reading and caching of entire 'stco', 'stsc', and
                   'stsz' chunks with partial reading and caching, and only
                   when needed to read stream data.  This effectively lifts
                   the maximum file size limit and reduces the initial file
                   parsing delay.  Also the initial file parsing delay
                   is now the same for all files, big or small.
02/18/03   rpw     Added AMR track and sample entry atom support
11/04/02   rpw     Created file.

===========================================================================*/

/* <EJECT> */
/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#include "target.h"             /* Target specific definitions            */
#include "comdef.h"             /* Definitions for byte, word, etc.       */

#include "videofmt_bs.h"        /* Video bitstream services               */

/* <EJECT> */
/*===========================================================================

                        DATA DECLARATIONS

===========================================================================*/

/* This is the maximum atom stack depth for the MP4 file format services. */
#define VIDEO_FMT_MP4R_ATOM_STACK_DEPTH  15

/* This is the maximum size of each input ping-pong buffer. */
#define VIDEO_FMT_MP4R_BUFFER_SIZE       1024

/* This is the number of entries cached per sample table atom. */
#define VIDEO_FMT_MP4R_TABLE_CACHE_SIZE  800

/* These are the flags defined for the tf_flags in the tfhd box*/
#define TF_FLAG_BASE_DATA_OFFSET_MASK          0x000001
#define TF_FLAG_SAMPLE_DESCRIPTION_INDEX_MASK  0x000002
#define TF_FLAG_DEFAULT_SAMPLE_DURATION_MASK   0x000008
#define TF_FLAG_DEFAULT_SAMPLE_SIZE_MASK       0x000010
#define TF_FLAG_DEFAULT_SAMPLE_FLAGS_MASK      0x000020

/* These are the flags defined for the tr_flags in the Track Fragment Run Box */
#define TR_FLAG_DATA_OFFSET_FLAG_MASK             0x000001
#define TR_FLAG_FIRST_SAMPLE_FLAG_MASK            0x000004
#define TR_FLAG_SAMPLE_DURATION_MASK              0x000100
#define TR_FLAG_SAMPLE_SIZE_MASK                  0x000200
#define TR_FLAG_SAMPLE_FLAG_MASK                  0x000400
#define TR_FLAG_SAMPLE_COMPOSITE_TIME_OFFSET_MASK 0x000800

#define FLAG_I_VOP     0x00000000

#define FLAG_P_VOP     0x00010000

/* This macro converts the given 32-bit value between host and network
** order.
*/
#define N2H(x) ((((uint32) (((uint8 *) &x) [0])) << 24) \
                | (((uint32) (((uint8 *) &x) [1])) << 16) \
                | (((uint32) (((uint8 *) &x) [2])) << 8) \
                | ((uint32) (((uint8 *) &x) [3])))

/* This function type defines the interface for processing a particular type
** of MP4 file atom.
*/
typedef boolean (*video_fmt_mp4r_atom_process_func_type)
(
  void  *context_ptr,
  void  *atom_struct
);

/* These are the different processing states of the MP4 file format services
** state machine when processing an MP4 file atom.
*/
typedef enum video_fmt_mp4r_process_atom_substate_type
{
  VIDEO_FMT_MP4R_PROCESS_ATOM_SUBSTATE_INIT,
  VIDEO_FMT_MP4R_PROCESS_ATOM_SUBSTATE_PARSE,
  VIDEO_FMT_MP4R_PROCESS_ATOM_SUBSTATE_CHILDREN,
  VIDEO_FMT_MP4R_PROCESS_ATOM_SUBSTATE_CHILDREN_DONE
} video_fmt_mp4r_process_atom_substate_type;

/* Generic structure of an atom field */
typedef struct video_fmt_atom_field_type
{
  uint32      offset;   /* offset of source field within atom structure */
  uint32      size;     /* size of field in bytes                       */
  uint8*      dest;     /* pointer to where field is to be stored       */
} video_fmt_atom_field_type;

typedef void (*video_fmt_mp4r_dest_process_func_type) (void  *context_ptr);

/* The structure of the 'consumable' part of an atom */
typedef struct video_fmt_consume_atom_structure_type
{
  video_fmt_mp4r_dest_process_func_type  dest_proc_func;
  uint32                                field_count;
  video_fmt_atom_field_type*            fields;
} video_fmt_consume_atom_structure_type;

/* This structure is used to keep track of a current position within an MP4
** file atom.
*/
typedef struct video_fmt_mp4r_atom_pos_type
{
  video_fmt_mp4_atom_type  atom;  /* describes atom details */

  /* This stores the current atom processing substate for the atom. */
  video_fmt_mp4r_process_atom_substate_type process_substate;

  /* When parsing a complex atom, these variables are used to track the
  ** current position and state of the parser.
  */
  uint32   parse_pos;
  boolean  parse_flags [2];
  uint32   parse_uint32 [7];

  /* When processing an atom, this is the pointer to the
  ** processing function.
  */
  video_fmt_mp4r_atom_process_func_type  atom_process_func;
  video_fmt_consume_atom_structure_type*  atom_struct;
} video_fmt_mp4r_atom_pos_type;

/* This structure represents one entry in a table grouping the attributes of
** different atom types.
*/
typedef struct video_fmt_mp4r_atom_info_type
{
  uint32                                  type;
  boolean                                 is_container;
  video_fmt_mp4r_atom_process_func_type   process_func;
  video_fmt_consume_atom_structure_type*  atom_struct;
} video_fmt_mp4r_atom_info_type;

/* These are the different states of the MP4 file format services parser state
** machine.
*/
typedef enum video_fmt_mp4r_state_type
{
  VIDEO_FMT_MP4R_STATE_INIT,
  VIDEO_FMT_MP4R_STATE_GET_DATA,
  VIDEO_FMT_MP4R_STATE_UPDATE_BUFFER,
  VIDEO_FMT_MP4R_STATE_DECODE_ATOM_HEADER,
  VIDEO_FMT_MP4R_STATE_DECODE_EXTENDED_ATOM_HEADER,
  VIDEO_FMT_MP4R_STATE_PROCESS_ATOM,
  VIDEO_FMT_MP4R_STATE_READY,
#ifdef FEATURE_FILE_FRAGMENTATION
  VIDEO_FMT_MP4R_STATE_PROCESS_FRAGMENT,
#endif
#if defined(FEATURE_FILE_FRAGMENTATION) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
  VIDEO_FMT_MP4R_STATE_UPDATE_SIZE_RETRIEVE_BUFFER,
#endif
#if defined (FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
  VIDEO_FMT_MP4R_STATE_RETRIEVE_FRAGMENT_SIZE,
#endif
  VIDEO_FMT_MP4R_STATE_INVALID
} video_fmt_mp4r_state_type;

/* These are the different states of the MP4 file format services stream state
** machine.
*/
typedef enum video_fmt_mp4r_stream_state_type
{
  VIDEO_FMT_MP4R_STREAM_STATE_GET_DATA,
  VIDEO_FMT_MP4R_STREAM_STATE_READY,
  VIDEO_FMT_MP4R_STREAM_STATE_STSZ_COUNT,
  VIDEO_FMT_MP4R_STREAM_STATE_READ,
  VIDEO_FMT_MP4R_STREAM_STATE_GET_SAMPLE_INFO,
  VIDEO_FMT_MP4R_STREAM_STATE_FIND_SYNC_SAMPLE,
  VIDEO_FMT_MP4R_STREAM_STATE_TRUN_COUNT,
  VIDEO_FMT_MP4R_STREAM_STATE_FIND_ABS_OFFSET,
#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  VIDEO_FMT_MP4R_STREAM_STATE_GET_ACCESS_POINT,
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/
  VIDEO_FMT_MP4R_STREAM_STATE_INVALID
} video_fmt_mp4r_stream_state_type;

/* Definition of the sample table types
*/
typedef enum
{
  VIDEO_FMT_MP4R_STSZ_TABLE,
  VIDEO_FMT_MP4R_STCO_TABLE,
  VIDEO_FMT_MP4R_STSS_TABLE,
  VIDEO_FMT_MP4R_STSC_TABLE,
  VIDEO_FMT_MP4R_STSC_INFO_TABLE,
  VIDEO_FMT_MP4R_STTS_TABLE,
#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  VIDEO_FMT_MP4R_TFRA_TABLE,
#endif/*FEATURE_QTV_RANDOM_ACCESS_REPOS*/
  VIDEO_FMT_MP4R_INVALID_TABLE
} video_fmt_mp4r_table_type;

/* Definition of the buffer types
*/
typedef enum
{
  VIDEO_FMT_MP4R_IN_BUFFER,
  VIDEO_FMT_MP4R_SIZE_RETRIEVE_BUFFER
} video_fmt_mp4r_buffer_type;

/*  Definition of the fields of the 'stsc' table entries
*/
typedef struct
{
  uint32 first_chunk;
  uint32 samples_per_chunk;
  uint32 sample_desc_index;
} video_fmt_mp4r_stsc_entry_type;

/*  Definition of the fields of the 'stsz' table entries
*/
typedef struct
{
  uint32 sample_size;
} video_fmt_mp4r_stsz_entry_type;

/*  Definition of the fields of the 'stco' table entries
*/
typedef struct
{
  uint32 chunk_offset;
} video_fmt_mp4r_stco_entry_type;

/*  Definition of the fields of the 'stss' table entries
*/
typedef struct
{
  uint32 sync_sample;
} video_fmt_mp4r_stss_entry_type;

/*  Definition of the fields of the 'stts' table entries
*/
typedef struct
{
  uint32 count;
  uint32 delta;
} video_fmt_mp4r_stts_entry_type;

#ifdef FEATURE_FILE_FRAGMENTATION

/* This structure contains information in one entry of the sample table
** trun atom
*/
typedef struct
{
    uint32 entry_one;
} video_fmt_mp4r_trun_one_entry_type;

typedef struct
{
    uint32 entry_one;
    uint32 entry_two;
} video_fmt_mp4r_trun_two_entry_type;

typedef struct
{
    uint32 entry_one;
    uint32 entry_two;
    uint32 entry_three;
} video_fmt_mp4r_trun_three_entry_type;

typedef struct
{
    uint32 entry_one;
    uint32 entry_two;
    uint32 entry_three;
    uint32 entry_four;
} video_fmt_mp4r_trun_four_entry_type;

typedef struct
{
    uint32 sample_duration;
    uint32 sample_size;
    uint32 sample_flags;
    uint32 sample_composition_time_offset;
} video_fmt_mp4r_trun_entry_type;

typedef enum video_fmt_mp4r_trun_sample_combination_type
{
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_PRESENT = 0x000100,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_SIZE_PRESENT = 0x000200,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_FLAGS_PRESENT = 0x000400,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_COMP_TIME_OFFSET_PRESENT = 0x000800,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_AND_SIZE_PRESENT = 0x000300,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_AND_FLAGS_PRESENT = 0x000400,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_AND_COMP_TIME_OFFSET_PRESENT = 0x000900,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_SIZE_AND_FLAGS_PRESENT = 0x000600,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_SIZE_AND_COMP_TIME_OFFSET_PRESENT = 0x000A00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_FLAGS_AND_COMP_TIME_OFFSET_PRESENT = 0x000C00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_SIZE_AND_FLAGS_PRESENT = 0x000700,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_SIZE_AND_COMP_TIME_OFFSET_PRESENT = 0x000B00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_DURATION_FLAGS_AND_COMP_TIME_OFFSET_PRESENT = 0x000D00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_SIZE_FLAGS_AND_COMP_TIME_OFFSET_PRESENT = 0x000E00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_ALL_PRESENT = 0x000F00,
  VIDEO_FMT_MP4R_TRUN_SAMPLE_INVALID
} video_fmt_mp4r_trun_sample_combination_type;
#endif

/*  Definition of the generic attributes of a sample table
*/
typedef struct
{
  uint32 file_offset;
  uint32 table_size;
  int32 current_table_pos;
  int32 cache_start;
  uint32 cache_size;
#ifdef FEATURE_FILE_FRAGMENTATION
  uint32 tr_flags;
  uint32 first_sample_flags;
  int64  data_offset;
  ////////////////////////
  /*
  * Each TRUN will have it's own values for following variables.
  * These values could be from TRUN header or default values from TFHD.
  * The reason each TRUN stores these values as there could be multiple TRAF for a given track
  * in a fragment, thus there could be multiple TFHD in a given fragment.
  */
  int64  base_data_offset;
  uint32 default_sample_size;
  uint32 default_sample_duration;
  ////////////////////////
  uint32 sample_description_index;
  uint32 numOptTrunFields;
  boolean tr_flag_first_sample_flag_present;
  video_fmt_mp4r_trun_sample_combination_type trun_sample_combination;
  boolean tr_flag_sample_duration_present;
  boolean tr_flag_sample_size_present;
  boolean tr_flag_sample_flags_present;
  boolean tr_flag_sample_composition_time_offset_present;
#endif/*FEATURE_FILE_FRAGMENTATION*/
#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  uint32 track_id;
  uint2  length_size_of_traf_num;
  uint2  length_size_of_trun_num;
  uint2  length_size_of_sample_num;
#endif/*FEATURE_QTV_RANDOM_ACCESS_REPOS*/
} video_fmt_mp4r_sample_table_type;

/* This structure contains all the information the video format services needs
** in order to maintain state about the parsing of an individual MP4 file
** media stream.
*/
typedef struct video_fmt_mp4r_stream_type
{
  /* This flag is set to TRUE on little-endian targets. */
  boolean byte_swap_needed;

  /* This is the status callback function pointer registered by
  ** the client.
  */
  video_fmt_status_cb_func_type callback_ptr;

  /* This is the client data passed to the format services, used to tell
  ** whether a request is currently active or not.
  */
  void *client_data;

#ifdef FEATURE_QTV_3GPP_PROGRESSIVE_DNLD
  uint32 wBufferOffset; /*Set by the client. Stream sample reads should not go
                        beyond this value.*/
#endif

  /* This tracks the current state of the MP4 file format services stream
  ** state machine.
  */
  video_fmt_mp4r_stream_state_type state;

  /* For transition states, this keeps track of the next states after the
  ** current state finishes.
  */
  video_fmt_mp4r_stream_state_type state_next [2];

  /* This flag indicates whether or not an end of file can be expected when
  ** getting input data from the client.
  */
  boolean expect_eof;

  /* This union is used to pass information back to the client through the
  ** status callback function.
  */
  video_fmt_status_cb_info_type  cb_info;

  /* This is the size of the header information for the stream.  It is copied
  ** from the stream information structure for the stream, because it is also
  ** needed internally by the stream state machine.
  */
  uint32 header;         /* # bytes in decSpecificInfo           */

  /* The following define each one of the sample tables parsed */
  video_fmt_mp4r_sample_table_type stsz;
  video_fmt_mp4r_sample_table_type stco;
  video_fmt_mp4r_sample_table_type stss;
  video_fmt_mp4r_sample_table_type stsc;
  video_fmt_mp4r_sample_table_type stts;
  video_fmt_mp4r_sample_table_type stsc_info;
#ifdef FEATURE_FILE_FRAGMENTATION
  int32 current_trun;      /* trun currently processed */
  uint32 trun_entry_count;  /* number of entries in the trun array */
  video_fmt_mp4r_sample_table_type  trun[VIDEO_FMT_MAX_RUNS];
  uint32 trun_sample_cache_trun_loaded; /* ??? */
#endif
#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  video_fmt_mp4r_sample_table_type  tfra;
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/

  /* These arrays cache a part of the sample table for the stream. */
  video_fmt_mp4r_stsz_entry_type   stsz_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_stco_entry_type   stco_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_stss_entry_type   stss_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_stsc_entry_type   stsc_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_stts_entry_type   stts_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_stsc_entry_type   stsc_info_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
#ifdef FEATURE_FILE_FRAGMENTATION
  video_fmt_mp4r_trun_entry_type        trun_cache[VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_trun_one_entry_type    trun_one_entry_cache[VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_trun_two_entry_type    trun_two_entry_cache[VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_trun_three_entry_type  trun_three_entry_cache[VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  video_fmt_mp4r_trun_four_entry_type   trun_four_entry_cache[VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
  boolean                               initialize_trun;
  boolean                               fill_trun_cache;
#endif
#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
   video_fmt_tfra_entry_type  tfra_cache [VIDEO_FMT_MP4R_TABLE_CACHE_SIZE];
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/
  /* While walking through the chunk and sample information in the sample
  ** table, these keep track of the current positions in the various parts of
  ** the sample table.
  */
  uint32 stsc_sample_countdown;

  /* These cache the sample offset, byte offset, and size in bytes of the
  ** current chunk being iterated in the VIDEO_FMT_MP4R_STATE_STREAM_READ
  ** state.
  */
  uint32 chunk_start_sample;
  uint32 chunk_start_byte;
  uint32 chunk_count_bytes;
  boolean chunk_count_bytes_valid;
  boolean chunk_adj_start_byte; /* this is used to indicate that rewind is in progress and we have to
                                   substract chunk_count_bytes from chunk_start_byte, to get actual
                                   chunk_start_byte */

  /* The number is the entry count in the 'stsd' container atom. */
  uint32 stsd_entry_count;

  /* These cache the offset into the current chunk where the next sample
  ** begins, after the last sample read in the
  ** VIDEO_FMT_MP4R_STATE_STREAM_READ state.
  */
  uint32 last_read_sample;
  uint32 last_read_offset;

  /* These cache the timestamp and byte offset of the current sample being
  ** iterated in the VIDEO_FMT_MP4R_STATE_STREAM_GET_SAMPLE_INFO or
  ** VIDEO_FMT_MP4R_STREAM_STATE_FIND_SYNC_SAMPLE states, as well as the
  ** number of samples accounted for in the current 'stts' table entry.
  */
  uint32 sample_timestamp;
  uint32 sample_byte_offset;
  uint32 sample_delta_count;

  /* This is the size of each sample.  If zero, the 'stsz' table needs to be
  ** consulted to find size of any particular sample in the file.  Otherwise,
  ** there is no 'stsz' table, and every sample has the same size, indicated
  ** here.
  */
  uint32 sample_size;

  /* This is the duration of each sample.  If zero, the 'stts' table needs to be
  ** consulted to find the duration of any particular sample in the file.  Otherwise,
  ** every sample has the same duration, indicated here.
  */
  uint32 sample_duration;

  /* If the stream has header information in the sample description that
  ** technically belongs at the start of the bitstream, this variable caches
  ** the location of this information.
  */
  uint32 header_location;

  /* These variables are used by the VIDEO_FMT_MP4R_STREAM_STATE_GET_DATA
  ** state to track the progress of getting data from the application layer.
  */
  uint8  *get_data_dst;
  uint32 get_data_src;
  uint32 get_data_size;
  uint32 get_data_needed;
  uint32 get_data_read;

  /* These variables are used by the VIDEO_FMT_MP4R_STATE_STREAM_STSZ_COUNT
  ** state to count the bytes in a sequential group of samples in a stream.
  */
  uint32  count_sample;
  uint32  count_sample_end;
  uint32  *count_bytes;

  /* These variables track state information for when a stream is being read
  ** by the client.
  */
  uint32                    read_size;          /* units left */
  video_fmt_data_unit_type  read_unit;          /* unit type */
  uint32                    read_offset;        /* stream offset in units */
  uint32                    read_header_offset; /* amt of header read */
  uint8                     *read_buffer;       /* where to store output */
  uint32                    read_total_bytes;   /* total bytes read */
  uint32                    read_amt_samples;   /* samples about to be read */
  uint32                    read_amt_bytes;     /* bytes about to be read */
  boolean                   read_amt_bytes_set;

  /* These variables track state information for when information about
  ** samples in a stream are being collected for the client.
  */
  uint32                      get_sample_info_stream;
  uint32                      get_sample_info_size;
  uint32                      get_sample_info_offset;
  video_fmt_sample_info_type  *get_sample_info_buffer;
  uint32                      get_sample_info_total_samples;

  /* This flag indicates the direction in which sync samples are sought in the
  ** current operation.
  */
  boolean                     seek_reverse;  /* TRUE = backward */

  /* If seeking sync samples, this flag indicates whether the start point has
  ** been visited yet or not.
  */
  boolean                     seek_start_found;

#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  /* These variables track state information for when information about
  ** access point in a stream are being collected for the client.
  */
  uint32                      get_access_point_timestamp;
  int32                       get_access_point_skipnumber;
  int32                       tfra_skip;
  video_fmt_tfra_entry_type   *get_access_point_buffer;
  boolean                     seek_access_point_reverse;
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/

#ifdef FEATURE_FILE_FRAGMENTATION
  /* This flag indicates the whether the fragment is being processed in the
  ** stream.
  */
  boolean                     fragment_processing;
  uint32                      main_fragment_frames;
  uint32                      main_fragment_bytes;
  uint32                      main_fragment_timestamp;
  uint32                      last_fragment_frames;
  uint32                      last_fragment_bytes;
  uint32                      last_fragment_timestamp;
  uint32                      last_sample_delta;
  uint32                      last_sample_offset;
  uint32                      default_sample_description_index;
  uint32                      default_sample_duration;
  uint32                      default_sample_size;
  uint32                      tf_flags;
  uint32                      default_sample_flags;
  int64                       base_data_offset;
#endif

  /* These variables allow the Annex I modification
  for stsc atom first_chunk starting from 0 */
  uint32                      first_entry_offset;
  boolean                     first_entry_offset_set;
#ifdef FEATURE_QTV_INTER_FRAG_REPOS
  boolean                     fragment_repositioned;
#endif
} video_fmt_mp4r_stream_type;

#ifdef FEATURE_FILE_FRAGMENTATION/*Support when "mvex" atom parse when it is before the "trak" atom*/
typedef struct trex_data_type
{
  uint32                      track_id;
  uint32                      default_sample_description_index;
  uint32                      default_sample_duration;
  uint32                      default_sample_size;
  uint32                      default_sample_flags;
}trex_data_type;
#endif
/* This structure contains all the state information for the MP4 file format
** services functions.
*/
typedef struct video_fmt_mp4r_context_type
{
  /* This is the status callback function pointer registered by
  ** the client.
  */
  video_fmt_status_cb_func_type callback_ptr;

  /* This is the client data passed to the format services, used to tell
  ** whether a request is currently active or not.
  */
  void *client_data;

  /*Client playing context[audio/video/text]*/
  uint8 client_playContext;
  uint32 valid_track_id[VIDEO_FMT_MAX_MEDIA_STREAMS];
  uint8 valid_track_count;

  /* This tracks the current state of the MP4 file format services parser
  ** state machine.
  */
  video_fmt_mp4r_state_type state;

  /* For transition states, this keeps track of the next states after the
  ** current state finishes.
  */
  video_fmt_mp4r_state_type state_next [2];

  /* Contains user-specific data defined at the file level
  */
  video_fmt_file_info_type  file_level_data;

  /* This is a stack of MP4 atom locations, used to track where the format
  ** services is in decoding an MP4 file.
  */
  video_fmt_mp4r_atom_pos_type atom_stack [VIDEO_FMT_MP4R_ATOM_STACK_DEPTH];
  uint32                      atom_stack_top;

  /* These are the input ping-pong buffers used to receive data from the
  ** client.
  */
  uint8  in_buffers [2] [VIDEO_FMT_MP4R_BUFFER_SIZE];
  uint32 in_buffer_which;  /* which buffer currently in use    */
  uint32 in_buffer_size;   /* number of bytes in buffer        */
  uint32 in_buffer_pos;    /* offset of first byte in buffer   */


#if defined (FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
  /* These are the ping-pong buffers to retrieve the size of the required fragment
  */
  uint8  size_retrieve_buffers [2] [VIDEO_FMT_MP4R_BUFFER_SIZE];
  uint32 size_retrieve_buffer_which;  /* which buffer currently in use    */
  uint32 size_retrieve_buffer_size;   /* number of bytes in buffer        */
  uint32 size_retrieve_buffer_pos;    /* offset of first byte in buffer   */
#endif

  /* These variables are used by the VIDEO_FMT_MP4R_STATE_GET_DATA state to
  ** track the progress of getting data from the application layer.
  */
  uint8  *get_data_dst;
  uint32 get_data_src;

#ifdef FEATURE_QTV_3GPP_PROGRESSIVE_DNLD
  boolean get_data_src_in_mdat;
#endif //FEATURE_QTV_3GPP_PROGRESSIVE_DNLD

  uint32 get_data_size;
  uint32 get_data_needed;
  uint32 get_data_read;

  /* This flag indicates whether or not an end of file can be expected when
  ** getting input data from the client.
  */
  boolean expect_eof;

  boolean mdat_present;             /* This flag indicates if the mdat is present for this clip */
  boolean moov_present;             /* This flag indicates if moov atom is present for this clip */
  boolean moof_present;             /* This flag indicates if moof atom is present for this clip
                                       (always FALSE for non-fragmented files)*/

#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
  boolean mfra_present;            /* This flag indicates if the Random Access Atom exists for this clip */
  video_fmt_mp4r_sample_table_type  tfra; /*contains information about tfra box if it exists*/
  boolean eof_reached;
  uint32  file_size;                /* To find out File Size */
  boolean mfro_present;
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/

  /* This union is used to pass information back to the client through the
  ** status callback function.
  */
  video_fmt_status_cb_info_type  cb_info;

  /* This keeps track of the current absolute position within
  ** the MP4 file.
  */
  uint32 abs_pos;

#if defined (FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
  /* This keeps track of the current absolute position for finding the fragment size
  ** within the MP4 file.
  */
  uint32 abs_size_retrieve_pos;
#endif

  /* This flag is set to TRUE on little-endian targets. */
  boolean byte_swap_needed;

  /* These structures are used to track the current parser state and provide
  ** feedback to the application layer for each available media stream.
  */
  video_fmt_stream_info_type  stream_info  [VIDEO_FMT_MAX_MEDIA_STREAMS];
  video_fmt_mp4r_stream_type  stream_state [VIDEO_FMT_MAX_MEDIA_STREAMS];
  uint32                      num_streams;

  /* This flag is set only while in the midst of parsing a sample description
  ** atom, or one of its descendants.
  */
  boolean in_sample_description_atom;
  //moved out here to know which fragment number that we are currently parsing.
  uint32  current_sequence_number;  /* fragment identification */
#ifdef FEATURE_FILE_FRAGMENTATION
  /* This flag is set when Fragments are present in the file */
  boolean fragment_present;
  uint32  current_track_id;         /* track currently parsed, in a fragment */
#endif

#ifdef FEATURE_FILE_FRAGMENTATION/*Support when "mvex" atom parse when it is before the "trak" atom*/
  trex_data_type trex_data[VIDEO_FMT_MAX_MEDIA_STREAMS];
  uint32 trex_count;
#endif
#if defined (FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
  uint32 fragment_requested;       /* to return the size of the requested fragment */
  uint32 fragment_size;
  boolean fragment_size_found;
#endif

  /* These values cache information returned by the video bitstream services
  ** during its callback.
  */
  uint32                          bs_offset;
  uint32                          bs_size;
  video_fmt_bs_status_type        bs_status;
  video_fmt_bs_cont_cb_func_type  bs_cont;
  video_fmt_bs_end_cb_func_type   bs_end;
  void                            *bs_server_data;
  boolean                         bs_done;
  boolean                         bs_short_header;
  boolean                         bs_mp3_id_bit;
} video_fmt_mp4r_context_type;

/* <EJECT> */
/*---------------------------------------------------------------------------
** VIDEOFMT_MP4R Private Function Prototypes
**---------------------------------------------------------------------------
*/

/*===========================================================================

FUNCTION  video_fmt_mp4r_end

DESCRIPTION
  This function terminates the current MP4 file format session.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_end (void *server_data);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process

DESCRIPTION
  This function runs the MP4 format encode/decode state machine.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_process (void *server_data);

/*===========================================================================

FUNCTION  video_fmt_mp4r_stream_process

DESCRIPTION
  This function runs the MP4 format stream state machine.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stream_process (void *server_data);

/*===========================================================================

FUNCTION  video_fmt_mp4r_read_stream

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request data to be read from a stream in the media file.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_read_stream
(
  uint32                    stream_number,
  video_fmt_data_unit_type  unit,
  uint32                    offset,        /* in units */
  uint32                    size,          /* in units */
  uint8                     *buffer,
  void                      *server_data,
  video_fmt_status_cb_func_type  callback_ptr,
  void                           *client_data
);

#ifdef FEATURE_QTV_3GPP_PROGRESSIVE_DNLD
/*===========================================================================

FUNCTION  video_fmt_mp4r_abs_file_offset

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request absolute file offset of a stream sample in the media file.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_abs_file_offset
(
  uint32                         stream_number,
  uint32                         sampleOffset,        /* in units              */
  uint32                         sampleSize,          /* in units              */
  void                           *server_data,
  video_fmt_status_cb_func_type  callback_ptr,
  void                           *client_data
);
#endif //FEATURE_QTV_3GPP_PROGRESSIVE_DNLD

/*===========================================================================

FUNCTION  video_fmt_mp4r_get_sample_info

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request information about one or more samples in a stream in the media
  file.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_get_sample_info
(
  uint32                         stream_number,
  uint32                         offset,
  uint32                         size,
  video_fmt_sample_info_type     *buffer,
  void                           *server_data,
  video_fmt_status_cb_func_type  callback_ptr,
  void                           *client_data
);

#if defined (FEATURE_FILE_FRAGMENTATION) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
/*===========================================================================

FUNCTION  video_fmt_mp4r_parse_fragment

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request parsing of the fragment if it exists.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_parse_fragment
(
  void                        *server_data
);



#endif

/*===========================================================================

FUNCTION  video_fmt_mp4r_largest_frame_size

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request the size of the largest frame in the track.

DEPENDENCIES
  For Fragmented files this is returning only the largest Frame size among
  the fragments already parsed.

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_largest_frame_size
(
  void                        *server_data,
  uint32                       stream_num
);

#if defined (FEATURE_QTV_PSEUDO_STREAM) || defined(FEATURE_QTV_3GPP_PROGRESSIVE_DNLD)
/*===========================================================================

FUNCTION  video_fmt_mp4r_fragment_size

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request parsing of the fragment if it exists.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_fragment_size
(
  void                        *server_data,
  uint32                      fragment_number
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_data_incomplete

DESCRIPTION

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_data_incomplete
(
 video_fmt_mp4r_context_type  *context
 );

/*===========================================================================

FUNCTION  video_fmt_mp4r_read_atom_size_type

DESCRIPTION

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_atom_read_size_type
(
 video_fmt_mp4r_context_type  *context,
 uint8                        *atom_size,
 uint8                        *atom_type
 );
#endif
/*===========================================================================

FUNCTION  video_fmt_mp4r_find_sync_sample

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request a lookup of the nearest sync sample at or before/after a given
  sample in a stream in the media file.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_find_sync_sample
(
  uint32                         stream_number,
  uint32                         offset,
  boolean                        reverse,
  video_fmt_sample_info_type     *buffer,
  void                           *server_data,
  video_fmt_status_cb_func_type  callback_ptr,
  void                           *client_data
);

#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
/*===========================================================================

FUNCTION  video_fmt_mp4r_find_access_point

DESCRIPTION
  This function is given to the client as a callback.  It is called in order
  to request the random Access Point

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_find_access_point
(
  uint32                         stream_number,
  uint32                         sample_timestamp,
  int32                          skipNumber,
  boolean                        reverse,
  video_fmt_tfra_entry_type      *buffer,
  void                           *server_data,
  video_fmt_status_cb_func_type  callback_ptr,
  void                           *client_data
);
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/

/*===========================================================================

FUNCTION  video_fmt_mp4r_failure

DESCRIPTION
  This function is called whenever there is no more data to retrieve.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_failure
(
  video_fmt_mp4r_context_type  *context
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_decode_atom

DESCRIPTION
  This function determines which MP4 file atom was just parsed, and moves the
  state machine to process the atom accordingly.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_decode_atom
(
  video_fmt_mp4r_context_type  *context
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_finish_atom

DESCRIPTION
  This function pops the current MP4 file atom off the atom stack, and returns
  to processing the parent atom.  If there is no parent, it moves the MP4 file
  format services state machine to decode the next atom in the file.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_finish_atom
(
  video_fmt_mp4r_context_type  *context
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_read_buffer

DESCRIPTION
  This function moves the MP4 format services state machine as necessary to
  get the requested number of bytes into an input buffer.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the requested number of bytes is available in the active input
  buffer.

  If FALSE, not all the bytes are available, and the state machine needs to be
  run to retrieve the needed bytes.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_read_buffer
(
  video_fmt_mp4r_context_type  *context,
  uint32                      num_bytes,
  boolean                     expect_eof,
  video_fmt_mp4r_buffer_type  buffer_type
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_consume_data

DESCRIPTION
  This function removes the given number of bytes from the MP4 format services
  input buffers, storing the data in the given destination buffer, and
  updating the state of the input buffers.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_consume_data
(
  video_fmt_mp4r_context_type  *context,
  uint8                       *destination,
  uint32                      amount,
  boolean                     byte_reverse,
  video_fmt_mp4r_buffer_type  buffer_type
);

/*===========================================================================

FUNCTION  video_fmt_consume_data

DESCRIPTION
  This function removes the given number of bytes from the dynamic allocated
  buffer, storing the data in the given destination buffer, and
  updating the state.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern int video_fmt_consume_data
(
  uint8 *source,
  uint8 *destination,
  int size,
  int readpos,
  int readsize,
  boolean  byte_reverse,
  video_fmt_mp4r_context_type   *context
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_peek_data

DESCRIPTION
  This function peeks the given number of bytes from the MP4 format services
  input buffers, storing the data in the given destination buffer.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_peek_data
(
  video_fmt_mp4r_context_type  *context,
  uint8                       *destination,
  uint32                      amount,
  boolean                     byte_reverse
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_skip_data

DESCRIPTION
  This function advances the given number of bytes in the MP4 file being
  parsed.  Data skipped that is already in the input buffers is removed.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_skip_data
(
  video_fmt_mp4r_context_type  *context,
  uint32                      amount,
  video_fmt_mp4r_buffer_type  buffer_type
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_check_end_of_file

DESCRIPTION
  This function checks to see if end of file

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_check_end_of_file
(
  video_fmt_mp4r_context_type  *context
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_empty

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes atoms which themselves are empty except
  for the other atoms they contain.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_empty
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_hdlr

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function identifies if the track contains Audio/Video and
  if it should further allow to continue parsing of the current track. It
  checks if the track is being parsed in the correct client play(audio/video)
  context.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_hdlr
(
  void *context_ptr,
  void *atom_struct_ptr
);
/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_skip

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes atoms which contain data we do not need
  to process, and will skip.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_skip
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_mvhd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'mvhd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_mvhd_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_tkhd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'tkhd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_tkhd_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_mdhd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'mdhd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_mdhd_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_damr_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'damr' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_damr_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_dawp_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'dawp' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_dawp_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_devc_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'devc' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_devc_prep_dest (void *context_ptr);

/*===========================================================================
FUNCTION  video_fmt_mp4r_decb_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'decb' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_decb_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_decw_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'decw' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_decw_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_dqcp_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'dqcp' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_dqcp_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_s263_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 's263' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
void video_fmt_mp4r_s263_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_d263_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'd263' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_d263_prep_dest (void *context_ptr);

#ifdef FEATURE_QTV_PDCF
/*===========================================================================
FUNCTION  video_fmt_mp4r_frma_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'frma' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_frma_prep_dest (void *context_ptr);


/*===========================================================================
FUNCTION  video_fmt_mp4r_schm_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'schm' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/

extern void video_fmt_mp4r_schm_prep_dest (void *context_ptr);


/*===========================================================================
FUNCTION  video_fmt_mp4r_ohdr_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'ohdr' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_ohdr_prep_dest (void *context_ptr);


/*===========================================================================
FUNCTION  video_fmt_mp4r_odaf_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'odaf' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_odaf_prep_dest (void *context_ptr);

#endif /* FEATURE_QTV_PDCF */

/*===========================================================================

FUNCTION  video_fmt_mp4r_stco_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'stco' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stco_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_stsc_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'stsc' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stsc_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_stts_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'stts' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stts_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_stss_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'stss' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stss_prep_dest (void *context_ptr);

/* <EJECT> */
/*===========================================================================

FUNCTION  video_fmt_mp4r_stsd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'stsd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_stsd_prep_dest (void *context_ptr);

#ifdef FEATURE_FILE_FRAGMENTATION
/*===========================================================================

FUNCTION  video_fmt_mp4r_mehd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'mehd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_mehd_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_trex_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'trex' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_trex_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_mfhd_prep_dest

DESCRIPTION
  This function prepares the destination fields where information
  from the 'mfhd' atom is to be stored.

DEPENDENCIES
  None

RETURN VALUE
  none

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_mfhd_prep_dest (void *context_ptr);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_tfhd

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'tfhd' atom, which contains a table
  of description common for all samples in all TRUN's.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_tfhd
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_trex

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'trex' atom, which contains a table
  of description common for all samples in all TRUN's.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_trex
(
  void *context_ptr,
  void *atom_struct_ptr
);

#endif /*FEATURE_FILE_FRAGMENTATION*/

/*===========================================================================

FUNCTION  video_fmt_mp4r_parse_atom

DESCRIPTION

  This particular function processes any specific atom.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_parse_atom
(
  void *context_ptr,
  void *atom_struct_pt
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_trak

DESCRIPTION

  This function parses a specific atom.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_trak
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_container

DESCRIPTION
  This function skips a container atom.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_container
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_stbl

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'stbl' atom, which contains the
  sample table for a track.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_stbl
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_tref_child

DESCRIPTION

  This function is one of many functions which process MP4 file atoms.

  This particular function parses the 'tref' children.
  The following atoms may be contained in the 'tref' container:
  'hint', 'dpnd', 'ipir', 'mpod', 'sync', which contain a track-id list.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_tref_child
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_tx3g

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'tx3g' atom, which contains
  timed text specific coding information.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_tx3g
(
  void *context_ptr,
  void *atom_struct_ptr
);

#ifdef FEATURE_QTV_H264
/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_avc1

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'avc1' atom, which contains
  advance video codec informations for H264 streams.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_avc1
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_avcC

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'avcC' atom, which contains
  advance video codec informations for H264 streams.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_avcC
(
  void *context_ptr,
  void *atom_struct_ptr
);
#endif /* FEATURE_QTV_H264 */

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_btrt

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'btrt' atom, which contains
  advance video codec informations for H264 streams.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_btrt
(
  void *context_ptr,
  void *atom_struct_ptr
);


/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_bitr

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'bitr' atom, which contains
  advance video codec informations for H263 streams.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_bitr
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_esds

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'esds' atom, which contains an
  elementary stream descriptor for a range of media samples.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_esds
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_stsz

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'stsz' atom, which contains a table
  of sizes for every sample in the corresponding elementary stream.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_stsz
(
  void *context_ptr,
  void *atom_struct_ptr
);

#ifdef FEATURE_FILE_FRAGMENTATION
/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_trun

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the 'trun' atom, which contains a table
  of sizes for every sample in the corresponding elementary stream.

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_process_atom_trun
(
  void *context_ptr,
  void *atom_struct_ptr
);
#endif

/*===========================================================================

FUNCTION  video_fmt_mp4r_process_atom_udta_child

DESCRIPTION
  This function is one of many functions which process MP4 file atoms.

  This particular function processes the atoms contained in the file or
  in the track 'udta' container atom:
    - 'vinf' atoms (at track level), which contain the video frame rate

DEPENDENCIES
  None

RETURN VALUE
  If TRUE, the MP4 file format services should return to the caller.

SIDE EFFECTS
  None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_udta_child
(
  void *context_ptr,
  void *atom_struct_ptr
);

/*===========================================================================

FUNCTION  video_fmt_mp4r_bs_callback

DESCRIPTION
  This function is the callback function from the video bitstream
  services.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_bs_callback
(
  video_fmt_bs_status_type          status,
  void                              *client_data,
  video_fmt_bs_status_cb_info_type  *info,
  video_fmt_bs_end_cb_func_type     end
);

/* <EJECT> */
/*===========================================================================

FUNCTION  video_fmt_mp4r_read_network_word

DESCRIPTION
  This function byte-swaps the given 4-byte word stored in network byte order
  (big-endian) if necessary so that it can be accessed by the native processor
  (host order, which may be big-endian or little-endian).

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern void video_fmt_mp4r_read_network_word
(
  video_fmt_mp4r_context_type *context,
  unsigned long *network_word
);

#ifdef FEATURE_QTV_RANDOM_ACCESS_REPOS
/*===========================================================================

FUNCTION  video_fmt_mp4r_check_mfra_box

DESCRIPTION
  This function checks to see if the mfra box exists and processes it

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None

===========================================================================*/
extern boolean video_fmt_mp4r_check_mfra_box
(
  video_fmt_mp4r_context_type  *context
);
#endif /*FEATURE_QTV_RANDOM_ACCESS_REPOS*/
/*===========================================================================
FUNCTION  video_fmt_mp4r_process_atom_mp3

DESCRIPTION
 This function consumes the .mp3 and ms\0u atoms. These atoms are not a part
of the normal MP4 file format but are usually encountered in files encoded
by ffmpeg. When these atoms are encountered, we set the codec type to mp3
audio and set other parametes to NULL. These parameters are set later by
reading the ES headers by the client application.

DEPENDENCIES
None

RETURN VALUE
None

SIDE EFFECTS
None

===========================================================================*/
boolean video_fmt_mp4r_process_atom_mp3
(
  void  *context_ptr,
  void  *atom_struct
);

#endif /* VIDEOFMT_MP4R_H */
