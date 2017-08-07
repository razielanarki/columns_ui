#include "stdafx.h"
#include "filter.h"

namespace filter_panel {
    void filter_panel_t::populate_list_from_chain(const metadb_handle_list_t<pfc::alloc_fast> & handles, bool b_last_in_chain)
    {
        //SendMessage(get_wnd(), WM_SETREDRAW, FALSE, NULL);
        bool b_redraw = disable_redrawing();
        pfc::list_t<pfc::string_simple_t<WCHAR> > previous_nodes;
        bool b_all_was_selected = false;
        if (m_nodes.get_count())
        {
            pfc::list_t<bool> sel_data;
            sel_data.set_count(m_nodes.get_count());
            bit_array_var_table selection(sel_data.get_ptr(), sel_data.get_count());
            get_selection_state(selection);
            t_size i, count = sel_data.get_count();
            b_all_was_selected = selection[0];
            for (i = 1; i<count; i++)
                if (selection[i])
                    previous_nodes.add_item(m_nodes[i].m_value);
        }
        //console::formatter() << "popc: " << t0.query_reset();
        populate_list(handles);
        //t0.query_reset();
        {
            t_size i, count = previous_nodes.get_count();
            pfc::array_t<bool> new_selection;
            new_selection.set_count(m_nodes.get_count());
            new_selection.fill_null();
            if (count || b_all_was_selected)
            {
                bool b_found = false;
                new_selection[0] = b_all_was_selected;
                for (i = 0; i<count; i++)
                {
                    t_size index;
                    if (mmh::partial_bsearch(m_nodes.get_count() - 1, m_nodes, node_t::g_compare, previous_nodes[i].get_ptr(), 1, index, get_sort_direction()))
                    {
                        new_selection[index] = true;
                        b_found = true;
                    }
                }
                if (!b_found)
                    new_selection[0] = true; //m_nodes.get_count() >= 1
                set_selection_state(bit_array_var_table(new_selection.get_ptr(), new_selection.get_count()),
                    bit_array_var_table(new_selection.get_ptr(), new_selection.get_count()), false);
                //if (b_last_in_chain)
                //    send_results_to_playlist();
                //else
                //    update_subsequent_filters();
            }
            else
            {
            }
        }
        if (b_redraw)
            enable_redrawing();
        //SendMessage(get_wnd(), WM_SETREDRAW, TRUE, NULL);
        //RedrawWindow(get_wnd(), NULL, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
        //console::formatter() << "popc: " << t0.query_reset();
    }

    void filter_panel_t::_on_items_added(const metadb_handle_list_t<pfc::alloc_fast_aggressive> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);


        metadb_handle_list actualHandles = handles;
        metadb_handle_list_t<pfc::alloc_fast_aggressive> handlesNotifyNext;
        handlesNotifyNext.prealloc(actualHandles.get_count());

        m_nodes[0].m_handles.add_items(actualHandles);

        bool b_no_selection = get_selection_count(1) == 0 || get_item_selected(0);

        {
            pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> data0;
            //data0.prealloc(handles.get_count());


            get_data_entries_v2(actualHandles.get_ptr(), actualHandles.get_count(), data0, g_showemptyitems);

            mmh::Permuation permutation(data0.get_count());
            mmh::sort_get_permuation(data0.get_ptr(), permutation, data_entry_t::g_compare, false, get_sort_direction(), true);

            pfc::list_permutation_t<data_entry_t> data2(data0, permutation.get_ptr(), permutation.get_count());
            pfc::list_base_const_t<data_entry_t> & data = data2;

            pfc::list_t<uih::ListView::InsertItem, pfc::alloc_fast_aggressive> items;
            items.prealloc(data.get_count());

            {
                if (!m_field_data.is_empty())
                {


                    data_entry_t * p_data = data0.get_ptr();
                    t_size * perm = permutation.get_ptr();

                    //node_t node;
                    t_size i, count = data.get_count(), counter = 0;

                    for (i = 0; i<count; i++)
                        if (i + 1 == count || !((p_data[perm[i]].m_same_as_next = !StrCmpI(p_data[perm[i]].m_text.get_ptr(), p_data[perm[i + 1]].m_text.get_ptr()))))
                            counter++;

                    for (i = 0; i<count; i++)
                    {
                        t_size start = i;
                        while (p_data[perm[i]].m_same_as_next && i + 1<count)
                            i++;
                        t_size handles_count = 1 + i - start, k;

                        t_size index_item;
                        bool b_exact = mmh::partial_bsearch(m_nodes.get_count() - 1, m_nodes, node_t::g_compare, 
                            p_data[perm[start]].m_text.get_ptr(), 1, index_item, get_sort_direction());
                        if (b_exact)
                        {
                            t_size current_count = m_nodes[index_item].m_handles.get_count();
                            m_nodes[index_item].m_handles.set_count(current_count + handles_count);

                            bool b_selected = !b_no_selection && get_item_selected(index_item);

                            for (k = 0; k<handles_count; k++)
                                m_nodes[index_item].m_handles[current_count + k] = p_data[perm[start + k]].m_handle;

                            if (b_selected && handles_count)
                                handlesNotifyNext.add_items_fromptr(m_nodes[index_item].m_handles.get_ptr() + current_count, handles_count);
                        }
                        else
                        {
                            node_t node;
                            node.m_value = p_data[perm[start]].m_text.get_ptr();
                            node.m_handles.set_count(handles_count);

                            for (k = 0; k<handles_count; k++)
                                node.m_handles[k] = p_data[perm[start + k]].m_handle;

                            m_nodes.insert_item(node, index_item);
                            InsertItem item;
                            insert_items(index_item, 1, &item);
                        }
                    }

                    update_first_node_text(true);
                }
            }
        }
        if (index + 1<windows.get_count())
        {
            //if (index==0)
            //    g_update_subsequent_filters(windows, index+1, false, false);
            if (b_no_selection)
                windows[index + 1]->_on_items_added(actualHandles);
            else if (handlesNotifyNext.get_count())
                windows[index + 1]->_on_items_added(handlesNotifyNext);
        }
    }

    void filter_panel_t::_on_items_removed(const metadb_handle_list_t<pfc::alloc_fast_aggressive> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);

        metadb_handle_list_t<pfc::alloc_fast_aggressive> handlesNotifyNext;
        handlesNotifyNext.prealloc(handles.get_count());

        bool b_no_selection = get_selection_count(1) == 0 || get_item_selected(0);
        {
            auto& existing_handles = m_nodes[0].m_handles;
            const auto existing_handle_count = existing_handles.get_count();
            mmh::Permuation perm(existing_handle_count);
            pfc::array_staticsize_t<bool> remove_mask(existing_handle_count);
            pfc::fill_array_t(remove_mask, false);
            mmh::sort_get_permuation(existing_handles, perm, pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, false);

            const size_t remove_handles_count = handles.get_count();
            for (size_t j = 0; j < remove_handles_count; j++) {
                size_t handle_index = -1;
                if (existing_handles.bsearch_permutation_t(pfc::compare_t<metadb_handle_ptr, metadb_handle_ptr>, handles[j], perm, handle_index)) {
                    remove_mask[handle_index] = true;
                }
            }
            existing_handles.remove_mask(remove_mask.get_ptr());
        }

        {
            pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> data0;
            //data0.prealloc(handles.get_count());


            get_data_entries_v2(handles, data0, g_showemptyitems);

            mmh::Permuation permutation(data0.get_count());
            mmh::sort_get_permuation(data0.get_ptr(), permutation, data_entry_t::g_compare, false, get_sort_direction(), true);

            pfc::list_permutation_t<data_entry_t> data2(data0, permutation.get_ptr(), permutation.get_count());
            pfc::list_base_const_t<data_entry_t> & data = data2;

            {
                if (!m_field_data.is_empty())
                {

                    data_entry_t * p_data = data0.get_ptr();
                    t_size * perm = permutation.get_ptr();

                    //node_t node;
                    t_size i, count = data.get_count();

                    for (i = 0; i + 1 < count; i++)
                        p_data[perm[i]].m_same_as_next = !StrCmpI(p_data[perm[i]].m_text.get_ptr(), p_data[perm[i + 1]].m_text.get_ptr());

                    pfc::array_t<bool> mask0;
                    mask0.set_count(m_nodes.get_count());
                    mask0.fill_null();


                    for (i = 0; i<count; i++)
                    {
                        t_size start = i;
                        while (p_data[perm[i]].m_same_as_next && i + 1<count)
                            i++;
                        t_size handles_count = 1 + i - start;

                        t_size index_item;
                        bool b_exact = mmh::partial_bsearch(m_nodes.get_count() - 1, m_nodes, node_t::g_compare, 
                            p_data[perm[start]].m_text.get_ptr(), 1, index_item, get_sort_direction());

                        if (b_exact)
                        {
                            //t_size current_count = m_nodes[index_item].m_handles.get_count();
                            //m_nodes[index_item].m_handles.set_count(handles_count);

                            bool b_selected = !b_no_selection && get_item_selected(index_item);

                            t_size k;
                            for (k = 0; k<handles_count; k++)
                            {
                                m_nodes[index_item].m_handles.remove_item(p_data[perm[start + k]].m_handle);
                                if (b_selected) handlesNotifyNext.add_item(p_data[perm[start + k]].m_handle);
                            }


                            if (m_nodes[index_item].m_handles.get_count() == 0)
                            {
                                mask0[index_item] = true;
                            }
                        }
                    }
                    m_nodes.remove_mask(mask0.get_ptr());
                    remove_items(bit_array_table(mask0.get_ptr(), mask0.get_size()));
                    update_first_node_text(true);
                }
            }
        }
        //if (index==0)
        //    g_update_subsequent_filters(windows, index+1, false, false);
        if (index + 1<windows.get_count())
        {
            if (b_no_selection)
                windows[index + 1]->_on_items_removed(handles);
            else if (handlesNotifyNext.get_count())
                windows[index + 1]->_on_items_removed(handlesNotifyNext);
        }

    }

    void filter_panel_t::on_items_added(const pfc::list_base_const_t<metadb_handle_ptr> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);
        if (index == 0 || index == pfc_infinite)
        {
            _on_items_added(metadb_handle_list_t<pfc::alloc_fast_aggressive>(handles));
        }
    }
    void filter_panel_t::on_items_removed(const pfc::list_base_const_t<metadb_handle_ptr> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);
        if (index == 0 || index == pfc_infinite)
        {
            _on_items_removed(metadb_handle_list_t<pfc::alloc_fast_aggressive>(handles));
        }
    }

    void filter_panel_t::_on_items_modified(const metadb_handle_list_t<pfc::alloc_fast_aggressive> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);

        pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> data0;//, dataFilter;
        data0.prealloc(handles.get_count());


        metadb_handle_list actualHandles = handles;
        metadb_handle_list_t<pfc::alloc_fast_aggressive> handlesNotifyNext;
        handlesNotifyNext.prealloc(actualHandles.get_count());

        get_data_entries_v2(actualHandles.get_ptr(), actualHandles.get_count(), data0, g_showemptyitems);

        mmh::Permuation permutation(data0.get_count());
        mmh::sort_get_permuation(data0.get_ptr(), permutation, data_entry_t::g_compare, false, get_sort_direction(), true);

        pfc::list_permutation_t<data_entry_t> data2(data0, permutation.get_ptr(), permutation.get_count());
        pfc::list_base_const_t<data_entry_t> & data = data2;

        bool b_no_selection = get_selection_count(1) == 0 || get_item_selected(0);

        {
            if (!m_field_data.is_empty())
            {
                {
                    t_size j, countj = handles.get_count();
                    t_size k, countk = m_nodes.get_count();
                    for (k = 1; k<countk; k++)
                    {
                        pfc::array_t<bool> mask;
                        mask.set_count(m_nodes[k].m_handles.get_count());
                        mask.fill_null();
                        for (j = 0; j<countj; j++)
                        {
                            t_size index_found = m_nodes[k].m_handles.find_item(handles[j]);
                            if (index_found != pfc_infinite)
                                mask[index_found] = true;
                        }
                        m_nodes[k].m_handles.remove_mask(mask.get_ptr());
                    }
                }

                data_entry_t * p_data = data0.get_ptr();
                t_size * perm = permutation.get_ptr();

                t_size i, count = data.get_count();

                for (i = 0; i + 1 < count; i++)
                    p_data[perm[i]].m_same_as_next = !StrCmpI(p_data[perm[i]].m_text.get_ptr(), p_data[perm[i + 1]].m_text.get_ptr());

                for (i = 0; i<count; i++)
                {
                    t_size start = i;
                    while (p_data[perm[i]].m_same_as_next && i + 1<count)
                        i++;
                    t_size handles_count = 1 + i - start, k;

                    t_size index_item;
                    bool b_exact = mmh::partial_bsearch(m_nodes.get_count() - 1, m_nodes, node_t::g_compare, p_data[perm[start]].m_text.get_ptr(), 1, index_item, get_sort_direction());
                    if (b_exact)
                    {
                        t_size current_count = m_nodes[index_item].m_handles.get_count();
                        m_nodes[index_item].m_handles.set_count(current_count + handles_count);

                        bool b_selected = !b_no_selection && get_item_selected(index_item);

                        for (k = 0; k<handles_count; k++)
                        {
                            m_nodes[index_item].m_handles[current_count + k] = p_data[perm[start + k]].m_handle;
                        }
                        if (b_selected && handles_count)
                            handlesNotifyNext.add_items_fromptr(m_nodes[index_item].m_handles.get_ptr() + current_count, handles_count);
                    }
                    else
                    {
                        node_t node;
                        node.m_value = p_data[perm[start]].m_text.get_ptr();
                        node.m_handles.set_count(handles_count);

                        for (k = 0; k<handles_count; k++)
                            node.m_handles[k] = p_data[perm[start + k]].m_handle;

                        m_nodes.insert_item(node, index_item);
                        InsertItem item;
                        insert_items(index_item, 1, &item, false);
                    }
                }

                {
                    t_size k, countk = m_nodes.get_count();
                    pfc::array_t<bool> mask0;
                    mask0.set_count(countk);
                    mask0[0] = false;
                    //mask0.fill_null();
                    for (k = 1; k<countk; k++)
                    {
                        mask0[k] = m_nodes[k].m_handles.get_count() == 0;
                    }
                    m_nodes.remove_mask(mask0.get_ptr());
                    remove_items(bit_array_table(mask0.get_ptr(), mask0.get_size()), true);
                }
                update_first_node_text(true);
            }
        }
        if (index + 1<windows.get_count())
        {
            if (b_no_selection)
                windows[index + 1]->_on_items_modified(actualHandles);
            else if (handlesNotifyNext.get_count())
                windows[index + 1]->_on_items_modified(handlesNotifyNext);
        }
    }

    void filter_panel_t::on_items_modified(const pfc::list_base_const_t<metadb_handle_ptr> & handles)
    {
        pfc::ptr_list_t<filter_panel_t> windows;
        get_windows(windows);
        t_size index = windows.find_item(this);
        if (index == 0 || index == pfc_infinite)
        {
            metadb_handle_list_t<pfc::alloc_fast_aggressive> actualHandles = handles;
            _on_items_modified(actualHandles);
        }
    }

    void filter_panel_t::get_data_entries_v2(const pfc::list_base_const_t<metadb_handle_ptr> & handles, pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> & p_out, bool b_show_empty)
    {
        metadb_handle_list p_handles(handles);
        get_data_entries_v2(p_handles.get_ptr(), p_handles.get_count(), p_out, b_show_empty);
    }

    void filter_panel_t::get_data_entries_v2(const metadb_handle_ptr * p_handles, t_size count, pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> & p_out, bool b_show_empty)
    {
        class handle_info_t
        {
        public:
            metadb_info_container::ptr m_info;
            t_size m_value_count;
            t_size m_field_index;
        };
        //pfc::stringcvt::string_wide_from_utf8_t<pfc::alloc_fast_aggressive> str_utf16;
        if (!m_field_data.is_empty())
        {

            if (m_field_data.m_use_script)
            {
                titleformat_object_wrapper to(m_field_data.m_script);
                p_out.set_count(count);
                data_entry_t * pp_out = p_out.get_ptr();
                std::atomic<size_t> node_count{0};
                concurrency::parallel_for(size_t{0}, count, [&node_count, &to, p_handles, b_show_empty, pp_out](size_t i) {
                    pfc::string8_fastalloc buffer;
                    buffer.prealloc(32);
                    p_handles[i]->format_title(nullptr, buffer, to, nullptr);
                    if (b_show_empty || pfc::strlen_max(buffer, 1))
                    {
                        const size_t node_index = node_count++;
                        pp_out[node_index].m_handle = p_handles[i];
                        pp_out[node_index].m_text.set_size(pfc::stringcvt::estimate_utf8_to_wide_quick(buffer));
                        pfc::stringcvt::convert_utf8_to_wide_unchecked(pp_out[node_index].m_text.get_ptr(), buffer);
                        //int size = LCMapString(LOCALE_USER_DEFAULT, LCMAP_SORTKEY, pp_out[k].m_text.get_ptr(), -1, NULL, NULL);
                        //pp_out[k].m_sortkey.set_size(size);
                        //LCMapString(LOCALE_USER_DEFAULT, LCMAP_SORTKEY, pp_out[k].m_text.get_ptr(), -1, (LPWSTR)pp_out[k].m_sortkey.get_ptr(), size);
                    }
                });
                p_out.set_count(node_count);
            }
            else
            {
                pfc::list_t<handle_info_t> infos;
                infos.set_count(count);
                handle_info_t * p_infos = infos.get_ptr();

                t_size counter = 0, lcount = m_field_data.m_fields.get_count();

                for (size_t i{0}; i < count; i++)
                {
                    if (p_handles[i]->get_info_ref(p_infos[i].m_info))
                    {
                        for (size_t l{0}; l < lcount; l++)
                        {
                            p_infos[i].m_field_index = p_infos[i].m_info->info().meta_find(m_field_data.m_fields[l]);
                            p_infos[i].m_value_count = p_infos[i].m_field_index != pfc_infinite ? p_infos[i].m_info->info().meta_enum_value_count(p_infos[i].m_field_index) : 0;
                            counter += p_infos[i].m_value_count;
                            if (p_infos[i].m_value_count)
                                break;
                        }
                    }
                    else p_infos[i].m_value_count = 0;
                }

                p_out.set_count(counter);

                data_entry_t * pp_out = p_out.get_ptr();
                std::atomic<size_t> out_counter{0};

                concurrency::parallel_for(size_t{0}, count, [&out_counter, p_handles, p_infos, b_show_empty, pp_out](size_t i)
                {
                    t_size j;
                    for (j = 0; j<p_infos[i].m_value_count; j++)
                    {
                        const char * str = p_infos[i].m_info->info().meta_enum_value(p_infos[i].m_field_index, j);
                        if (b_show_empty || *str)
                        {
                            size_t out_index = out_counter++;
                            pp_out[out_index].m_handle = p_handles[i];
                            pp_out[out_index].m_text.set_size(pfc::stringcvt::estimate_utf8_to_wide_quick(str));
                            pfc::stringcvt::convert_utf8_to_wide_unchecked(pp_out[out_index].m_text.get_ptr(), str);
                            //int size = LCMapString(LOCALE_USER_DEFAULT, LCMAP_SORTKEY, pp_out[k].m_text.get_ptr(), -1, NULL, NULL);
                            //pp_out[k].m_sortkey.set_size(size);
                            //LCMapString(LOCALE_USER_DEFAULT, LCMAP_SORTKEY, pp_out[k].m_text.get_ptr(), -1, (LPWSTR)pp_out[k].m_sortkey.get_ptr(), size);

                        }
                    }
                });
                p_out.set_count(out_counter);
            }
        }
    }

    void filter_panel_t::populate_list(const metadb_handle_list_t<pfc::alloc_fast> & handles)
    {
        clear_all_items();
        m_nodes.remove_all();

        //m_nodes.prealloc(handles.get_count());

        pfc::list_t<data_entry_t, pfc::alloc_fast_aggressive> data0;
        //data0.prealloc(handles.get_count());

        const metadb_handle_list & actualHandles = handles;

        {
            get_data_entries_v2(actualHandles.get_ptr(), actualHandles.get_size(), data0, g_showemptyitems);
        }

        mmh::Permuation permutation(data0.get_count());
        {
            mmh::sort_get_permuation(data0.get_ptr(), permutation, data_entry_t::g_compare, false, get_sort_direction(), true);
        }

        //data.reorder(permutation.get_ptr());
        pfc::list_permutation_t<data_entry_t> data2(data0, permutation.get_ptr(), permutation.get_count());
        pfc::list_base_const_t<data_entry_t> & data = data2;

        pfc::list_t<uih::ListView::InsertItem, pfc::alloc_fast_aggressive> items;
        items.prealloc(data.get_count());
        {
            {
                data_entry_t * p_data = data0.get_ptr();
                t_size * perm = permutation.get_ptr();
                const size_t count{data.get_count()};

                concurrency::combinable<size_t> counts;
                concurrency::parallel_for(size_t{0}, count, [&counts, perm, p_data, count](size_t i)
                {
                    if (i + 1 == count || !(p_data[perm[i]].m_same_as_next = !StrCmpI(p_data[perm[i]].m_text.get_ptr(), p_data[perm[i + 1]].m_text.get_ptr())))
                        ++counts.local();
                });

                m_nodes.set_count(counts.combine(std::plus<size_t>()) + 1);
                node_t * p_nodes = m_nodes.get_ptr();
                {
                    p_nodes[0].m_handles.add_items(actualHandles);
                    p_nodes[0].m_value.set_string(L"All");
                }

                for (size_t i{0}, j{1}; i < count; i++)
                {
                    const size_t start{i};
                    while (p_data[perm[i]].m_same_as_next && i + 1<count)
                        i++;
                    const size_t handles_count{1 + i - start};

                    PFC_ASSERT(j < m_nodes.get_count());

                    p_nodes[j].m_handles.set_count(handles_count);
                    for (t_size k{0}; k < handles_count; k++)
                        p_nodes[j].m_handles[k] = p_data[perm[start + k]].m_handle;
                    p_nodes[j].m_value = p_data[perm[start]].m_text.get_ptr();
                    j++;
                }
                update_first_node_text();
            }
        }
        items.set_count(m_nodes.get_count());
        insert_items(0, items.get_count(), items.get_ptr());
    }

    void filter_panel_t::notify_sort_column(t_size index, bool b_descending, bool b_selection_only)
    {
        auto node_count = m_nodes.get_count();
        if (node_count > 2) {
            mmh::Permuation sort_permuation(node_count - 1);
            const auto * nodes = m_nodes.get_ptr();
            ++nodes;
            mmh::sort_get_permuation(nodes, sort_permuation, node_t::g_compare_ptr_with_node, false, b_descending, true);

            m_nodes.reorder_partial(1, sort_permuation.get_ptr(), node_count - 1);

            reorder_items_partial(1, sort_permuation.get_ptr(), node_count - 1);
            ensure_visible(get_focus_item());

            size_t field_index;
            if (cfg_field_list.find_by_name(m_field_data.m_name, field_index)) {
                cfg_field_list[field_index].m_last_sort_direction = b_descending;
            }
        }
    }

}