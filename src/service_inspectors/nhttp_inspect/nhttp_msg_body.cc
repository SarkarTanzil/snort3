//--------------------------------------------------------------------------
// Copyright (C) 2014-2015 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// nhttp_msg_body.cc author Tom Peters <thopeter@cisco.com>

#include <string.h>
#include <sys/types.h>
#include <stdio.h>

#include "detection/detection_util.h"
#include "file_api/file_api.h"
#include "mime/file_mime_process.h"

#include "nhttp_enum.h"
#include "nhttp_msg_request.h"
#include "nhttp_msg_body.h"

using namespace NHttpEnums;

NHttpMsgBody::NHttpMsgBody(const uint8_t* buffer, const uint16_t buf_size,
    NHttpFlowData* session_data_, SourceId source_id_, bool buf_owner, Flow* flow_) :
    NHttpMsgSection(buffer, buf_size, session_data_, source_id_, buf_owner, flow_),
    data_length(session_data->data_length[source_id]),
    body_octets(session_data->body_octets[source_id])
{
    transaction->set_body(this);
}

void NHttpMsgBody::analyze()
{
    data.start = msg_text.start;
    data.length = msg_text.length;

    // Always set file data. File processing will later set a new value in some cases.
    if (data.length > 0)
    {
        set_file_data(const_cast<uint8_t*>(data.start), (unsigned)data.length);
    }

    if (session_data->file_depth_remaining[source_id] > 0)
    {
        do_file_processing();
    }

    body_octets += msg_text.length;

    if (tcp_close && (body_octets < data_length))
        infractions += INF_TRUNCATED;
}

void NHttpMsgBody::do_file_processing()
{
    // Using the trick that cutter is deleted when regular or chunked body is complete
    const bool front = (body_octets == 0);
    const bool back = (session_data->cutter[source_id] == nullptr) || tcp_close;
    FilePosition file_position;
    if (front && back) file_position = SNORT_FILE_FULL;
    else if (front) file_position = SNORT_FILE_START;
    else if (back) file_position = SNORT_FILE_END;
    else file_position = SNORT_FILE_MIDDLE;

    // Chunked body with nothing but the zero length chunk?
    if (front && (data.length == 0))
    {
        return;
    }

    const int32_t fp_length = (data.length <= session_data->file_depth_remaining[source_id]) ?
        data.length : session_data->file_depth_remaining[source_id];

    if (source_id == SRC_SERVER)
    {
        if (file_api->file_process(flow, const_cast<uint8_t*>(data.start), fp_length,
            file_position, false, false))
        {
            session_data->file_depth_remaining[source_id] -= fp_length;

            // With the first piece of the file we must provide the "name" which means URI
            if (front)
            {
                NHttpMsgRequest* request = transaction->get_request();
                if (request != nullptr)
                {
                    const Field& tranaction_uri = request->get_uri_norm_legacy();
                    if (tranaction_uri.length > 0)
                    {
                        file_api->set_file_name(flow, const_cast<uint8_t*>(tranaction_uri.start),
                            tranaction_uri.length);
                    }
                }
            }
        }
        else
        {
            // file processing doesn't want any more data
            session_data->file_depth_remaining[source_id] = 0;
        }
    }
    else if (session_data->mime_state != nullptr)
    {
        session_data->mime_state->process_mime_data(flow, data.start, data.start + fp_length,
            true, file_position);

        session_data->file_depth_remaining[source_id] -= fp_length;
        if (session_data->file_depth_remaining[source_id] == 0)
        {
            delete(session_data->mime_state);
            session_data->mime_state = nullptr;
        }
    }
}

void NHttpMsgBody::gen_events()
{
}

void NHttpMsgBody::print_section(FILE* output)
{
    NHttpMsgSection::print_message_title(output, "body");
    fprintf(output, "Expected data length %" PRIi64 ", octets seen %" PRIi64 "\n", data_length,
        body_octets);
    data.print(output, "Data");
    NHttpMsgSection::print_message_wrapup(output);
}

void NHttpMsgBody::update_flow()
{
    if (body_octets < data_length)
    {
        // More body coming
        session_data->body_octets[source_id] = body_octets;
        session_data->section_size_target[source_id] = DATA_BLOCK_SIZE;
        session_data->section_size_max[source_id] = FINAL_BLOCK_SIZE;
        session_data->infractions[source_id] = infractions;
        session_data->events[source_id] = events;
    }
    else
    {
        // End of message
        session_data->type_expected[source_id] = (source_id == SRC_CLIENT) ? SEC_REQUEST :
            SEC_STATUS;
        session_data->half_reset(source_id);
    }
    session_data->section_type[source_id] = SEC__NOTCOMPUTE;
}

