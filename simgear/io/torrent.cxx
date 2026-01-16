/*
SPDX-License-Identifier: LGPL-2.0-or-later
SPDX-FileCopyrightText: 2025 Julian Smith
*/

/* We define nothing unless SG_TORRENT is defined. */
#ifdef SG_TORRENT

#include <simgear/debug/logstream.hxx>
#include <simgear/io/sg_file.hxx>
#include <simgear/props/props.hxx>

#include "torrent.hxx"

#include "libtorrent/alert_types.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/session_stats.hpp"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/torrent_info.hpp"

#include <algorithm>
#include <iostream>


namespace simgear
{
const char* Torrent::staticSubsystemClassId()
{
    return "torrent";
}
}

/*
On Debian/Devuan Linux we require:

    apt install libtorrent-rasterbar-dev
*/


namespace simgear
{

static SGPropertyNode_ptr s_node;

static std::function<simgear::HTTP::Client* ()> s_get_http_client;

static libtorrent::session* s_session = nullptr;
static int s_incoming_connections = 0;
static int s_incoming_requests = 0;
static std::mutex s_mutex;
static std::map<libtorrent::torrent_handle, std::function<void (bool ok)>> s_torrent_to_callback;
static std::map<libtorrent::torrent_handle, SGPropertyNode_ptr> s_torrent_to_node;
static time_t s_torrent_status_t0 = 0;
static std::vector<libtorrent::stats_metric>   s_stats_metrics;
static std::map<std::string, libtorrent::torrent_handle> s_stg_leafname_to_torrent_handle;
static std::map<SGPath, libtorrent::torrent_handle> s_torrent_path_to_torrent_handle;

Torrent::Torrent(
        SGPropertyNode_ptr node,
        std::function<simgear::HTTP::Client* ()> get_http_client
        )
{
    s_node = node;
    s_get_http_client = get_http_client;
}

void Torrent::init()
{
    assert(!s_session);
    
    s_session = new libtorrent::session;
    s_incoming_connections = 0;
    s_incoming_requests = 0;
    
    /* Set alert mask. */
    libtorrent::settings_pack   settings_pack;
    settings_pack.set_int(
            libtorrent::settings_pack::alert_mask,
            0
            | libtorrent::alert_category::error
            | libtorrent::alert_category::peer
            | libtorrent::alert_category::port_mapping
            | libtorrent::alert_category::storage
            | libtorrent::alert_category::tracker
            | libtorrent::alert_category::connect
            | libtorrent::alert_category::status
            | libtorrent::alert_category::ip_block
            | libtorrent::alert_category::performance_warning
            | libtorrent::alert_category::dht
            | libtorrent::alert_category::incoming_request
            //| libtorrent::alert_category::dht_operation   // very verbose.
            | libtorrent::alert_category::port_mapping_log
            | libtorrent::alert_category::file_progress
            );
    s_session->apply_settings(settings_pack);
    s_stats_metrics = libtorrent::session_stats_metrics();
}

void Torrent::shutdown()
{
    delete s_session;
    s_session = nullptr;
    s_torrent_to_callback.clear();
    s_torrent_to_node.clear();
    s_torrent_status_t0 = 0;
    s_stats_metrics.clear();
    
}

void Torrent::unbind()
{
    /* Delete our properties. */
    s_node->removeAllChildren();
    s_node = nullptr;
}


static std::string endpoint_to_string(const libtorrent::tcp::endpoint& endpoint)
{
    std::ostringstream out;
    out << endpoint;
    return out.str();
}


/* Write torrent_status information to property tree. */
static void internal_update_torrent_status(const libtorrent::torrent_status& torrent_status, SGPropertyNode_ptr torrent_node)
{
    /* Commented-out lines are for data that is not a number or string; will
    eventually convert to text that can be shown as a property value. */
    
    SGPropertyNode* torrent_status_node = torrent_node->getNode("status");
    //torrent_status_node->setIntValue("errc", torrent_status.errc);
    torrent_status_node->setIntValue("error_file", torrent_status.error_file);
    torrent_status_node->setStringValue("save_path", torrent_status.save_path);
    torrent_status_node->setStringValue("name", torrent_status.name);
    //torrent_status_node->setIntValue("next_announce", torrent_status.next_announce);
    torrent_status_node->setStringValue("current_tracker", torrent_status.current_tracker);
    torrent_status_node->setIntValue("total_download", torrent_status.total_download);
    torrent_status_node->setIntValue("total_upload", torrent_status.total_upload);
    torrent_status_node->setIntValue("total_payload_download", torrent_status.total_payload_download);
    torrent_status_node->setIntValue("total_payload_upload", torrent_status.total_payload_upload);
    torrent_status_node->setIntValue("total_failed_bytes", torrent_status.total_failed_bytes);
    torrent_status_node->setIntValue("total_redundant_bytes", torrent_status.total_redundant_bytes);
    torrent_status_node->setIntValue("total_done", torrent_status.total_done);
    torrent_status_node->setIntValue("total", torrent_status.total);
    torrent_status_node->setIntValue("total_wanted_done", torrent_status.total_wanted_done);
    torrent_status_node->setIntValue("total_wanted", torrent_status.total_wanted);
    torrent_status_node->setIntValue("all_time_upload", torrent_status.all_time_upload);
    torrent_status_node->setIntValue("all_time_download", torrent_status.all_time_download);
    torrent_status_node->setIntValue("added_time", torrent_status.added_time);
    torrent_status_node->setIntValue("completed_time", torrent_status.completed_time);
    torrent_status_node->setIntValue("last_seen_complete", torrent_status.last_seen_complete);
    torrent_status_node->setIntValue("storage_mode", torrent_status.storage_mode);
    torrent_status_node->setFloatValue("progress", torrent_status.progress);
    torrent_status_node->setIntValue("progress_ppm", torrent_status.progress_ppm);
    torrent_status_node->setIntValue("queue_position", torrent_status.queue_position);
    torrent_status_node->setIntValue("download_rate", torrent_status.download_rate);
    torrent_status_node->setIntValue("upload_rate", torrent_status.upload_rate);
    torrent_status_node->setIntValue("download_payload_rate", torrent_status.download_payload_rate);
    torrent_status_node->setIntValue("upload_payload_rate", torrent_status.upload_payload_rate);
    torrent_status_node->setIntValue("num_seeds", torrent_status.num_seeds);
    torrent_status_node->setIntValue("num_peers", torrent_status.num_peers);
    torrent_status_node->setIntValue("num_complete", torrent_status.num_complete);
    torrent_status_node->setIntValue("num_incomplete", torrent_status.num_incomplete);
    torrent_status_node->setIntValue("list_seeds", torrent_status.list_seeds);
    torrent_status_node->setIntValue("list_peers", torrent_status.list_peers);
    torrent_status_node->setIntValue("connect_candidates", torrent_status.connect_candidates);
    torrent_status_node->setIntValue("num_pieces", torrent_status.num_pieces);
    torrent_status_node->setIntValue("distributed_full_copies", torrent_status.distributed_full_copies);
    torrent_status_node->setIntValue("distributed_fraction", torrent_status.distributed_fraction);
    torrent_status_node->setFloatValue("distributed_copies", torrent_status.distributed_copies);
    torrent_status_node->setIntValue("block_size", torrent_status.block_size);
    torrent_status_node->setIntValue("num_uploads", torrent_status.num_uploads);
    torrent_status_node->setIntValue("num_connections", torrent_status.num_connections);
    torrent_status_node->setIntValue("uploads_limit", torrent_status.uploads_limit);
    torrent_status_node->setIntValue("connections_limit", torrent_status.connections_limit);
    torrent_status_node->setIntValue("up_bandwidth_queue", torrent_status.up_bandwidth_queue);
    torrent_status_node->setIntValue("down_bandwidth_queue", torrent_status.down_bandwidth_queue);
    torrent_status_node->setIntValue("seed_rank", torrent_status.seed_rank);
    torrent_status_node->setIntValue("state", torrent_status.state);
    torrent_status_node->setBoolValue("need_save_resume", torrent_status.need_save_resume);
    torrent_status_node->setBoolValue("is_seeding", torrent_status.is_seeding);
    torrent_status_node->setBoolValue("is_finished", torrent_status.is_finished);
    torrent_status_node->setBoolValue("has_metadata", torrent_status.has_metadata);
    torrent_status_node->setBoolValue("has_incoming", torrent_status.has_incoming);
    torrent_status_node->setBoolValue("moving_storage", torrent_status.moving_storage);
    torrent_status_node->setBoolValue("announcing_to_trackers", torrent_status.announcing_to_trackers);
    torrent_status_node->setBoolValue("announcing_to_lsd", torrent_status.announcing_to_lsd);
    torrent_status_node->setBoolValue("announcing_to_dht", torrent_status.announcing_to_dht);
    //torrent_status_node->setBoolValue("info_hashes", torrent_status.info_hashes);
    //torrent_status_node->setIntValue("last_upload", torrent_status.last_upload);
    //torrent_status_node->setIntValue("last_download", torrent_status.last_download);
    //torrent_status_node->setIntValue("active_duration", torrent_status.active_duration);
    //torrent_status_node->setIntValue("finished_duration", torrent_status.finished_duration);
    //torrent_status_node->setIntValue("seeding_duration", torrent_status.seeding_duration);
    //torrent_status_node->setIntValue("seeding_duration", torrent_status.seeding_duration);
    torrent_status_node->setIntValue("flags", torrent_status.flags);
}


/* Write peer_info information to property tree. */
static void internal_update_torrent_peer(const libtorrent::peer_info& peer_info, SGPropertyNode_ptr peers_node)
{
    // https://libtorrent.org/reference-Core.html#peer_info

    /* We use peer_info.ip as property directory. We need `ip_` prefix and
    convert ':' to '_' to obey property name rules. */
    std::string peer_ip = "ip_" + endpoint_to_string(peer_info.ip);
    std::replace(peer_ip.begin(), peer_ip.end(), ':', '_');
    SGPropertyNode* peer_node = peers_node->getNode(peer_ip, true /*create*/);

    peer_node->setStringValue("client", peer_info.client);
    peer_node->setIntValue("flags", peer_info.flags);
    peer_node->setIntValue("source", peer_info.source);

    peer_node->setIntValue("up_speed", peer_info.up_speed);
    peer_node->setIntValue("down_speed", peer_info.down_speed);
    peer_node->setIntValue("payload_up_speed", peer_info.payload_up_speed);
    peer_node->setIntValue("payload_down_speed", peer_info.payload_down_speed);
    //peer_node->setIntValue("pid", peer_info.pid); // don't know what type this is (digest32<160>?)
    peer_node->setIntValue("queue_bytes", peer_info.queue_bytes);
    peer_node->setIntValue("request_timeout", peer_info.request_timeout);
    peer_node->setIntValue("send_buffer_size", peer_info.send_buffer_size);
    peer_node->setIntValue("used_send_buffer", peer_info.used_send_buffer);
    peer_node->setIntValue("receive_buffer_size", peer_info.receive_buffer_size);
    peer_node->setIntValue("used_receive_buffer", peer_info.used_receive_buffer);
    peer_node->setIntValue("receive_buffer_watermark", peer_info.receive_buffer_watermark);
    peer_node->setIntValue("num_hashfails", peer_info.num_hashfails);
    peer_node->setIntValue("download_queue_length", peer_info.download_queue_length);
    peer_node->setIntValue("timed_out_requests", peer_info.timed_out_requests);
    peer_node->setIntValue("busy_requests", peer_info.busy_requests);
    peer_node->setIntValue("requests_in_buffer", peer_info.requests_in_buffer);
    peer_node->setIntValue("target_dl_queue_length", peer_info.target_dl_queue_length);
    peer_node->setIntValue("upload_queue_length", peer_info.upload_queue_length);
    peer_node->setIntValue("failcountfailcount", peer_info.failcount);
    peer_node->setIntValue("downloading_piece_index", peer_info.downloading_piece_index);
    peer_node->setIntValue("downloading_block_index", peer_info.downloading_block_index);
    peer_node->setIntValue("downloading_progress", peer_info.downloading_progress);
    peer_node->setIntValue("downloading_total", peer_info.downloading_total);

    peer_node->setIntValue("connection_type", peer_info.connection_type);
    peer_node->setBoolValue("connection_type.standard_bittorrent", peer_info.connection_type & libtorrent::peer_info::standard_bittorrent);
    peer_node->setBoolValue("connection_type.web_seed", peer_info.connection_type & libtorrent::peer_info::web_seed);
    peer_node->setBoolValue("connection_type.http_seed", peer_info.connection_type & libtorrent::peer_info::http_seed);

    peer_node->setIntValue("pending_disk_bytes", peer_info.pending_disk_bytes);
    peer_node->setIntValue("pending_disk_read_bytes", peer_info.pending_disk_read_bytes);
    peer_node->setIntValue("send_quota", peer_info.send_quota);
    peer_node->setIntValue("receive_quota", peer_info.receive_quota);
    peer_node->setIntValue("rtt", peer_info.rtt);
    peer_node->setIntValue("num_pieces", peer_info.num_pieces);
    peer_node->setIntValue("download_rate_peak", peer_info.download_rate_peak);
    peer_node->setIntValue("upload_rate_peak", peer_info.upload_rate_peak);
    peer_node->setIntValue("progress", peer_info.progress);
    peer_node->setIntValue("progress_ppm", peer_info.progress_ppm);
    peer_node->setStringValue("local_endpoint", endpoint_to_string(peer_info.local_endpoint));
    peer_node->setIntValue("read_state", peer_info.read_state);
    peer_node->setIntValue("write_state", peer_info.write_state);
}


void Torrent::update(double /*delta_time_sec*/)
{
    /* Tell clients of success/failure of torrents by looking at libtorrent
    alerts. Also updates our properties. */
    std::vector<libtorrent::alert*> alerts;
    s_session->pop_alerts(&alerts);
    for (libtorrent::alert* alert: alerts)
    {
        if (0)
        {
        }
        else if (auto session_stats_alert = dynamic_cast<libtorrent::session_stats_alert*>(alert))
        {
            // Response from our call of s_session->post_session_stats().
            for (auto sm: s_stats_metrics)
            {
                s_node->setLongValue(
                        std::string() + "stats_metrics/" + sm.name,
                        session_stats_alert->counters()[sm.value_index]
                        );
            }
        }
        else if (auto state_update_alert = dynamic_cast<libtorrent::state_update_alert*>(alert))
        {
            for (const libtorrent::torrent_status& torrent_status: state_update_alert->status)
            {
                std::unique_lock    lock(s_mutex);
                auto torrent_node = s_torrent_to_node.find(torrent_status.handle)->second;
                lock.unlock();
                internal_update_torrent_status(torrent_status, torrent_node);
            }
        }
        else if (dynamic_cast<libtorrent::add_torrent_alert*>(alert))
        {
            /* We'd probably use this if we changed to use async_add_torrent(). */
        }
        else if (auto torrent_alert = dynamic_cast<libtorrent::torrent_alert*>(alert))
        {
            std::unique_lock    lock(s_mutex);
            auto torrent_node = s_torrent_to_node.find(torrent_alert->handle)->second;
            auto torrent_callback = s_torrent_to_callback.find(torrent_alert->handle)->second;
            lock.unlock();
            
            /* Schedule update of peer info here. Perhaps we should instead do
            this for all torrents regularly. */
            torrent_alert->handle.post_peer_info();
            
            if (0)
            {
            }
            else if (auto peer_info_alert = dynamic_cast<libtorrent::peer_info_alert*>(alert))
            {
                /* From torrent_handle::post_peer_info(). */
                SGPropertyNode* peers_node = torrent_node->getNode("peers", true /*create*/);
                for (const libtorrent::peer_info& peer_info: peer_info_alert->peer_info)
                {
                    internal_update_torrent_peer(peer_info, peers_node);
                }
            }
            else if (dynamic_cast<libtorrent::torrent_finished_alert*>(alert))
            {
                /* Update per-torrent stats when completed. */
                SG_LOG(SG_IO, SG_DEBUG, "torrent_finished_alert");
                torrent_node->setStringValue("status", "finished");
                if (torrent_callback)
                {
                    torrent_callback(true /*ok*/);
                }
            }
            else if (auto torrent_error_alert = dynamic_cast<libtorrent::torrent_error_alert*>(alert))
            {
                SG_LOG(SG_IO, SG_DEBUG, "torrent_error_alert"
                        << " typeid(*alert)=" << typeid(*alert).name()
                        << " torrent_error_alert->filename()=" << torrent_error_alert->filename()
                        << " torrent_error_alert->message()=" << torrent_error_alert->message()
                        );
                torrent_node->setStringValue("status", "error");
                torrent_node->setStringValue("error_filename", torrent_error_alert->filename());
                torrent_node->setStringValue("error_message", torrent_error_alert->message());
                /* I think this may end up making callback multiple times. If
                error occurs, libtorrent carries on trying. */
                if (torrent_callback)
                {
                    torrent_callback(false /*ok*/);
                }
            }
            else if (dynamic_cast<libtorrent::torrent_paused_alert*>(alert))
            {
                torrent_node->setBoolValue("paused", true);
            }
            else if (dynamic_cast<libtorrent::torrent_resumed_alert*>(alert))
            {
                torrent_node->setBoolValue("paused", false);
            }
            else if (dynamic_cast<libtorrent::torrent_checked_alert*>(alert))
            {
                torrent_node->setStringValue("status", "checked");
            }
            else if (auto torrent_log_alert = dynamic_cast<libtorrent::torrent_log_alert*>(alert))
            {
                /* "By default it is disabled as a build configuration." */
                SG_LOG(SG_IO, SG_ALERT, "torrent_log_alert: " << torrent_log_alert->log_message());
            }
        }
        else if (auto session_error_alert = dynamic_cast<libtorrent::session_error_alert*>(alert))
        {
            s_node->setStringValue("session_error", session_error_alert->message());
        }
        else if (dynamic_cast<libtorrent::incoming_connection_alert*>(alert))
        {
            s_incoming_connections += 1;
            s_node->setIntValue("incoming_connections", s_incoming_connections);
        }
        else if (dynamic_cast<libtorrent::incoming_request_alert*>(alert))
        {
            s_incoming_requests += 1;
            s_node->setIntValue("incoming_requests", s_incoming_requests);
        }
    }
    time_t t = time(nullptr);
    if (t - s_torrent_status_t0 >= 10)
    {
        s_torrent_status_t0 = t;
        // Ask for a libtorrent::session_stats_alert.
        SG_LOG(SG_IO, SG_DEBUG, "Calling s_session->post_session_stats()");
        s_session->post_session_stats();
        s_session->post_torrent_updates();
    }
}

void Torrent::add_torrent(
        SGPath& torrent_path,
        SGPath& out_path,
        Torrent::fn_result_callback result_callback
        )
{
    SG_LOG(SG_IO, SG_ALERT, "add_torrent():"
            << " torrent_path=" << torrent_path
            << " out_path=" << out_path
            );
    libtorrent::add_torrent_params add_torrent_params;
    /* Need to add torrent as not auto-managed and paused. Otherwise
    it can be unpaused and generate alerts before we have added it to
    s_torrent_to_callback. */
    add_torrent_params.flags |= libtorrent::torrent_flags::paused;
    add_torrent_params.flags &= (~libtorrent::torrent_flags::auto_managed);
    add_torrent_params.save_path = out_path.str();
    std::shared_ptr<libtorrent::torrent_info> torrent_info;
    try
    {
        /* This throws if torrent file is corrupt. */
        torrent_info = std::make_shared<libtorrent::torrent_info>(torrent_path.str());
    }
    catch (std::exception& e)
    {
        SG_LOG(SG_IO, SG_ALERT, "Failed to load torrent file '" + torrent_path.str() + "': " + e.what());
        if (result_callback)
        {
            result_callback(false /*ok*/);
        }
        return;
    }
    
    add_torrent_params.ti = torrent_info;
    
    /* Update our various maps to include the new torrent_handle. */
    std::unique_lock    lock(s_mutex);

    // todo: use async_add_torrent()?
    libtorrent::torrent_handle torrent_handle = s_session->add_torrent(add_torrent_params);

    {
        auto it = s_torrent_path_to_torrent_handle.find(torrent_path);
        if (it == s_torrent_path_to_torrent_handle.end())
        {
            /* This dummy entry was created by Torrent::add_torrent_url(). */
            s_torrent_path_to_torrent_handle[torrent_path] = torrent_handle;
        }
        else
        {
            assert(it->second == libtorrent::torrent_handle());
            it->second = torrent_handle;
        }
    }
    assert(s_torrent_to_callback.find(torrent_handle) == s_torrent_to_callback.end());

    s_torrent_to_callback[torrent_handle] = result_callback;

    const libtorrent::file_storage& file_storage = torrent_info->files();
    for (int i=0; i<file_storage.num_files(); ++i)
    {
        auto leafname = file_storage.file_name(i);
        std::string leafname2(leafname.data(), leafname.size());
        if (simgear::strutils::ends_with(leafname2, ".stg"))
        {
            assert(s_stg_leafname_to_torrent_handle.find(leafname2) == s_stg_leafname_to_torrent_handle.end());
            s_stg_leafname_to_torrent_handle[leafname2] = torrent_handle;

        }
    }

    SGPropertyNode_ptr torrent_node = s_node->addChild("torrent");
    torrent_node->setStringValue("status", "init");
    torrent_node->setStringValue("torrent", torrent_path.str());
    torrent_node->setStringValue("path", out_path.str());

    s_torrent_to_node[torrent_handle] = torrent_node;
    
    lock.unlock();

    /* It's now safe to allow torrent to generate alerts. */
    torrent_handle.set_flags(libtorrent::torrent_flags::auto_managed);
    torrent_handle.resume();
}


static void add_torrent_url_callback(
        Torrent* self,
        const std::string& torrent_url,
        SGPath& torrent_path,
        SGPath& out_path,
        Torrent::fn_result_callback result_callback,
        int code,
        const std::string& reason
        )
{
    SG_LOG(SG_IO, SG_ALERT, "add_torrent_url_callback():"
            << " code=" << code
            << " reason='" << reason << "'"
            << " torrent_url=" << torrent_url
            << " torrent_path=" << torrent_path
            );
    if (code)
    {
        if (result_callback)
        {
            result_callback(false);
        }
    }
    else
    {
        self->add_torrent(torrent_path, out_path, result_callback);
    }   
}


struct RequestFile : simgear::HTTP::Request
{
    /* Constructor. If we fail to open <path> we call <callback> with errno
    information then throw an exception. */
    RequestFile(
            const std::string& url,
            SGPath& path,
            std::function<void (int code, const std::string &reason)> callback
            )
    :
    simgear::HTTP::Request(url),
    m_callback(callback),
    m_file(path)
    {
        /* Always remove any existing file first, otherwise <mode> is ignored
        when we truncate. */
        path.remove();
        if (!m_file.open(SG_IO_OUT))
        {
            assert(errno);
            std::string message = "Failed to open: '" + path.str() + "': "
                    + simgear::strutils::error_string(errno);
            SG_LOG(SG_IO, SG_ALERT, message);
            callback(-errno, message);
            throw std::runtime_error(message);
        }
    }
    
    void gotBodyData(const char* s, int n) override
    {
        assert(n >= 0);
        while (n)
        {
            ssize_t nn = m_file.write(s, n);
            if (nn < 0)
            {
                /* Write failed. Cancel the download. */
                setFailure(-errno, simgear::strutils::error_string(errno));
                break;
            }
            assert(nn <= n);
            n -= nn;
        }
    }
    
    void finalResult(int code, const std::string& reason) override
    {
        SG_LOG(SG_IO, SG_DEBUG, "finalResult():"
                << " code=" << code
                << " reason='" << reason << "'"
                );
        if (!m_file.close())
        {
            SG_LOG(SG_IO, SG_ALERT, "finalResult(): failed to close m_file.");
        }
        m_callback(code, reason);
    }
    
    ~RequestFile()
    {
        assert(m_file.eof());    /* Set by m_file.close(). */
    }
    
    std::function<void (int code, const std::string& reason)> m_callback;
    SGFile m_file;
};

/* Download from <url> to <path>.

Makes a single call of <callback> on success/failure:

    On success we call callback() with code=0, and reason="".
    
    On failure we call callback() with one of:
        <code> set to -errno and <reason> from strerror().
        <code> and <reason> from libcurl.
*/
static void download_file(
        simgear::HTTP::Client* http_client,
        const std::string& url,
        SGPath& path,
        std::function<void (int code, const std::string& reason)> callback
        )
{
    SGSharedPtr<RequestFile> request;
    try
    {
        request = new RequestFile(url, path, callback);
    }
    catch(std::exception&)
    {
        return;
    }
    http_client->makeRequest(request);
}
            

void Torrent::add_torrent_url(
        const std::string& torrent_url,
        SGPath& torrent_path,
        SGPath& out_path,
        Torrent::fn_result_callback result_callback
        )
{
    SG_LOG(SG_IO, SG_ALERT, "[" << ((long) pthread_self()) << "]"
            << " Torrent::add_torrent_url():"
            << " torrent_url=" << torrent_url
            << " torrent_path=" << torrent_path
            << " out_path=" << out_path
            );
    simgear::HTTP::Client* http_client = s_get_http_client();
    assert(http_client);
    {
        std::unique_lock    lock(s_mutex);
        assert(s_torrent_path_to_torrent_handle.find(torrent_path) == s_torrent_path_to_torrent_handle.end());
        s_torrent_path_to_torrent_handle[torrent_path] = libtorrent::torrent_handle();
    }
    
    download_file(
            http_client,
            torrent_url,
            torrent_path,
            std::bind(
                add_torrent_url_callback,
                this,
                torrent_url,
                torrent_path,
                out_path,
                result_callback,
                std::placeholders::_1 /*code*/,
                std::placeholders::_2 /*reason*/
                )
            );
}


static std::ostream& operator<< (std::ostream& out, Torrent::status status)
{
    if (status == Torrent::status::NONE)
    {
        out << "NONE";
    }
    else if (status == Torrent::status::IN_PROGRESS)
    {
        out << "IN_PROGRESS";
    }
    else if (status == Torrent::status::DONE)
    {
        out << "DONE";
    }
    else
    {
        assert(0);
    }
    return out;
}


static Torrent::status s_torrent_handle_to_status(const libtorrent::torrent_handle& torrent_handle)
{
    Torrent::status ret;
    libtorrent::torrent_status torrent_status = torrent_handle.status(libtorrent::status_flags_t());
    SG_LOG(SG_IO, SG_DEBUG, "torrent_status=" << torrent_status.state);
    if (0
            || torrent_status.state == libtorrent::torrent_status::checking_files
            || torrent_status.state == libtorrent::torrent_status::downloading_metadata
            || torrent_status.state == libtorrent::torrent_status::downloading
            || torrent_status.state == libtorrent::torrent_status::finished
            || torrent_status.state == libtorrent::torrent_status::checking_resume_data
            )
    {
        ret = Torrent::status::IN_PROGRESS;
    }
    else if (torrent_status.state == libtorrent::torrent_status::seeding)
    {
        ret = Torrent::status::DONE;
    }
    else
    {
        SG_LOG(SG_IO, SG_DEBUG, "Unrecognised torrent state " << (int) torrent_status.state);
        assert(0);
    }
    return ret;
}

Torrent::status Torrent::get_status_torrent_path(const SGPath& torrent_path)
{
    Torrent::status ret;
    std::unique_lock    lock(s_mutex);
    auto it = s_torrent_path_to_torrent_handle.find(torrent_path);
    if (it == s_torrent_path_to_torrent_handle.end())
    {
        ret =  status::NONE;
    }
    else
    {
        const libtorrent::torrent_handle& torrent_handle = it->second;
        lock.unlock();
        if (torrent_handle == libtorrent::torrent_handle())
        {
            /* We are currently downloading the .torrent file itself using HTTP
            (see Torrent::add_torrent_url()). */
            ret = status::IN_PROGRESS;
        }
        else
        {
            ret = s_torrent_handle_to_status(torrent_handle);
        }
    }
    SG_LOG(SG_IO, SG_DEBUG, "Torrent::get_status_torrent_path() torrent_path=" << torrent_path
            << " returning " << ret);
    return ret;
}

Torrent::status Torrent::get_status_stg_leafname(const std::string& stg_leafname)
{
    Torrent::status ret;

    std::unique_lock    lock(s_mutex);
    auto it = s_stg_leafname_to_torrent_handle.find(stg_leafname);
    if (it == s_stg_leafname_to_torrent_handle.end())
    {
        ret =  status::NONE;
    }
    else
    {
        const libtorrent::torrent_handle& torrent_handle = it->second;
        lock.unlock();
        ret = s_torrent_handle_to_status(torrent_handle);
    }
    SG_LOG(SG_IO, SG_DEBUG, "Torrent::get_status_stg_leafname() stg_leafname=" << stg_leafname
            << " returning " << ret);
    return ret;
}

}   // namespace simgear.

#endif
