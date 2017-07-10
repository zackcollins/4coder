/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 25.02.2016
 *
 * File editing view for 4coder
 *
 */

// TOP

// Enriched Text

struct Enriched_Text{
    String source;
};

internal Enriched_Text
load_enriched_text(char *directory, char *filename){
    Enriched_Text result = {0};
    
    char space[256];
    String fname = make_fixed_width_string(space);
    append(&fname, directory);
    append(&fname, "/");
    append(&fname, filename);
    terminate_with_null(&fname);
    
    result.source = file_dump(fname.str);
    return(result);
}

// Document Declaration

enum{
    Doc_Root,
    Doc_Section,
    Doc_Todo,
    Doc_Enriched_Text,
    Doc_Element_List,
    Doc_Full_Elements,
    Doc_Table_Of_Contents
};

struct Alternate_Name{
    String macro;
    String public_name;
};

struct Alternate_Names_Array{
    Alternate_Name *names;
};

enum{
    AltName_Standard,
    AltName_Macro,
    AltName_Public_Name,
};

struct Document_Item{
    Document_Item *next;
    Document_Item *parent;
    i32 type;
    union{
        struct{
            Document_Item *first_child;
            Document_Item *last_child;
            String name;
            String id;
            i32 show_title;
        } section;
        
        struct{
            Meta_Unit *unit;
            Alternate_Names_Array *alt_names;
            i32 alt_name_type;
        } unit_elements;
        
        struct{
            Enriched_Text *text;
        } text;
    };
};
global Document_Item null_document_item = {0};

enum{
    ItemType_Document,
    ItemType_Image,
    ItemType_GenericFile,
    // never below this
    ItemType_COUNT,
};

struct Basic_Node{
    Basic_Node *next;
};

#define NodeGetData(node, T) ((T*) ((node)+1))

struct Basic_List{
    Basic_Node *head;
    Basic_Node *tail;
};

struct Abstract_Item{
    i32 item_type;
    char *name;
    
    // Document value members
    Document_Item *root_item;
    
    // TODO(allen): make these external
    // Document building members
    Document_Item *section_stack[16];
    i32 section_top;
    
    // Image value members
    char *source_file;
    char *extension;
    float w_h_ratio;
    float h_w_ratio;
    Basic_List img_instantiations;
};
global Abstract_Item null_abstract_item = {0};

struct Image_Instantiation{
    i32 w, h;
};

struct Document_System{
    Basic_List doc_list;
    Basic_List img_list;
    Basic_List file_list;
};

internal Document_System
create_document_system(){
    Document_System system = {0};
    return(system);
}

internal void*
push_item_on_list(Basic_List *list, i32 item_size){
    i32 mem_size = item_size + sizeof(Basic_Node);
    void *mem = fm__push(mem_size);
    Assert(mem != 0);
    memset(mem, 0, mem_size);
    
    Basic_Node *node = (Basic_Node*)mem;
    if (list->head == 0){
        list->head = node;
        list->tail = node;
    }
    else{
        list->tail->next = node;
        list->tail = node;
    }
    
    void *result = (node+1);
    return(result);
}

internal Abstract_Item*
get_item_by_name(Basic_List list, String name){
    Abstract_Item *result = 0;
    
    for (Basic_Node *node = list.head;
         node != 0;
         node = node->next){
        Abstract_Item *item = NodeGetData(node, Abstract_Item);
        if (match(item->name, name)){
            result = item;
            break;
        }
    }
    
    return(result);
}

internal Abstract_Item*
get_item_by_name(Basic_List list, char *name){
    Abstract_Item *result = 0;
    
    for (Basic_Node *node = list.head;
         node != 0;
         node = node->next){
        Abstract_Item *item = NodeGetData(node, Abstract_Item);
        if (match(item->name, name)){
            result = item;
            break;
        }
    }
    
    return(result);
}

internal Image_Instantiation*
get_image_instantiation(Basic_List list, i32 w, i32 h){
    Image_Instantiation *result = 0;
    
    for (Basic_Node *node = list.head;
         node != 0;
         node = node->next){
        Image_Instantiation *instantiation = NodeGetData(node, Image_Instantiation);
        if (instantiation->w == w && instantiation->h == h){
            result = instantiation;
            break;
        }
    }
    
    return(result);
}

internal Abstract_Item*
create_item(Basic_List *list, char *name){
    Abstract_Item *lookup = get_item_by_name(*list, name);
    
    Abstract_Item *result = 0;
    if (lookup == 0){
        result = (Abstract_Item*)push_item_on_list(list, sizeof(Abstract_Item));
    }
    
    return(result);
}

internal void
add_image_instantiation(Basic_List *list, i32 w, i32 h){
    Image_Instantiation *instantiation = (Image_Instantiation*)push_item_on_list(list, sizeof(Image_Instantiation));
    instantiation->w = w;
    instantiation->h = h;
}

internal void
set_section_name(Document_Item *item, char *name, i32 show_title){
    i32 name_len = str_size(name);
    item->section.name = make_string_cap(fm_push_array(char, name_len+1), 0, name_len+1);
    fm_align();
    append(&item->section.name, name);
    item->section.show_title = show_title;
}

internal void
set_section_id(Document_Item *item, char *id){
    i32 id_len = str_size(id);
    item->section.id = make_string_cap(fm_push_array(char, id_len+1), 0, id_len+1);
    fm_align();
    append(&item->section.id, id);
}

internal void
begin_document_description(Abstract_Item *doc, char *title, i32 show_title){
    *doc = null_abstract_item;
    doc->item_type = ItemType_Document;
    
    doc->root_item = fm_push_array(Document_Item, 1);
    *doc->root_item = null_document_item;
    doc->section_stack[doc->section_top] = doc->root_item;
    doc->root_item->type = Doc_Root;
    
    set_section_name(doc->root_item, title, show_title);
}

internal Abstract_Item*
add_generic_file(Document_System *system, char *source_file, char *extension, char *name){
    Abstract_Item *item = create_item(&system->file_list, name);
    if (item){
        item->item_type = ItemType_GenericFile;
        item->extension = extension;
        item->source_file = source_file;
        item->name = name;
    }
    return(item);
}

internal Abstract_Item*
add_image_description(Document_System *system, char *source_file, char *extension, char *name){
    Abstract_Item *item = create_item(&system->img_list, name);
    if (item != 0){
        item->item_type = ItemType_Image;
        item->extension = extension;
        item->source_file = source_file;
        item->name = name;
        
        i32 w = 0, h = 0, comp = 0;
        i32 stbi_r = stbi_info(source_file, &w, &h, &comp);
        if (!stbi_r){
            fprintf(stderr, "Did not find file %s\n", source_file);
            item->w_h_ratio = 1.f;
            item->h_w_ratio = 1.f;
        }
        else{
            item->w_h_ratio = ((float)w/(float)h);
            item->h_w_ratio = ((float)h/(float)w);
        }
    }
    return(item);
}

internal Abstract_Item*
begin_document_description(Document_System *system, char *title, char *name, i32 show_title){
    Abstract_Item *item = create_item(&system->doc_list, name);
    if (item){
        begin_document_description(item, title, show_title);
        item->name = name;
    }
    return(item);
}

internal void
end_document_description(Abstract_Item *item){
    Assert(item->section_top == 0);
}

internal void
append_child(Document_Item *parent, Document_Item *item){
    Assert(parent->type == Doc_Root || parent->type == Doc_Section);
    if (parent->section.last_child == 0){
        parent->section.first_child = item;
    }
    else{
        parent->section.last_child->next = item;
    }
    parent->section.last_child = item;
    item->parent = parent;
}

internal void
begin_section(Abstract_Item *item, char *title, char *id){
    Assert(item->section_top+1 < ArrayCount(item->section_stack));
    
    Document_Item *parent = item->section_stack[item->section_top];
    Document_Item *section = fm_push_array(Document_Item, 1);
    *section = null_document_item;
    item->section_stack[++item->section_top] = section;
    
    section->type = Doc_Section;
    
    set_section_name(section, title, 1);
    if (id != 0){
        set_section_id(section, id);
    }
    
    append_child(parent, section);
}

internal void
end_section(Abstract_Item *doc){
    Assert(doc->section_top > 0);
    --doc->section_top;
}

internal void
add_todo(Abstract_Item *doc){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Todo;
    
    append_child(parent, item);
}

internal void
add_element_list(Abstract_Item *doc, Meta_Unit *unit){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Element_List;
    item->unit_elements.unit = unit;
    
    append_child(parent, item);
}

internal void
add_element_list(Abstract_Item *doc, Meta_Unit *unit, Alternate_Names_Array *alt_names, i32 alt_name_type){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Element_List;
    item->unit_elements.unit = unit;
    item->unit_elements.alt_names = alt_names;
    item->unit_elements.alt_name_type = alt_name_type;
    
    append_child(parent, item);
}

internal void
add_full_elements(Abstract_Item *doc, Meta_Unit *unit){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Full_Elements;
    item->unit_elements.unit = unit;
    
    append_child(parent, item);
}

internal void
add_full_elements(Abstract_Item *doc, Meta_Unit *unit, Alternate_Names_Array *alt_names, i32 alt_name_type){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Full_Elements;
    item->unit_elements.unit = unit;
    item->unit_elements.alt_names = alt_names;
    item->unit_elements.alt_name_type = alt_name_type;
    
    append_child(parent, item);
}

internal void
add_table_of_contents(Abstract_Item *doc){
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Table_Of_Contents;
    
    append_child(parent, item);
}

internal void
add_enriched_text(Abstract_Item *doc, Enriched_Text *text){
    Assert(doc->section_top+1 < ArrayCount(doc->section_stack));
    
    Document_Item *parent = doc->section_stack[doc->section_top];
    Document_Item *item = fm_push_array(Document_Item, 1);
    *item = null_document_item;
    item->type = Doc_Enriched_Text;
    item->text.text = text;
    
    append_child(parent, item);
}

// HTML Document Generation

#define HTML_BACK_COLOR   "#FAFAFA"
#define HTML_TEXT_COLOR   "#0D0D0D"
#define HTML_CODE_BACK    "#DFDFDF"
#define HTML_EXAMPLE_BACK "#EFEFDF"

#define HTML_POP_COLOR_1  "#309030"
#define HTML_POP_BACK_1   "#E0FFD0"
#define HTML_VISITED_LINK "#A0C050"

#define HTML_POP_COLOR_2  "#005000"

#define HTML_CODE_STYLE "font-family: \"Courier New\", Courier, monospace; text-align: left;"

#define HTML_CODE_BLOCK_STYLE(back)                             \
"margin-top: 3mm; margin-bottom: 3mm; font-size: .95em; "  \
"background: "back"; padding: 0.25em;"

#define HTML_DESCRIPT_SECTION_STYLE HTML_CODE_BLOCK_STYLE(HTML_CODE_BACK)
#define HTML_EXAMPLE_CODE_STYLE HTML_CODE_BLOCK_STYLE(HTML_EXAMPLE_BACK)

#define HTML_DOC_HEAD_OPEN  "<div style='margin-top: 3mm; margin-bottom: 3mm; color: "HTML_POP_COLOR_1";'><b><i>"
#define HTML_DOC_HEAD_CLOSE "</i></b></div>"

#define HTML_DOC_ITEM_HEAD_STYLE "font-weight: 600;"

#define HTML_DOC_ITEM_HEAD_INL_OPEN  "<span style='"HTML_DOC_ITEM_HEAD_STYLE"'>"
#define HTML_DOC_ITEM_HEAD_INL_CLOSE "</span>"

#define HTML_DOC_ITEM_HEAD_OPEN  "<div style='"HTML_DOC_ITEM_HEAD_STYLE"'>"
#define HTML_DOC_ITEM_HEAD_CLOSE "</div>"

#define HTML_DOC_ITEM_OPEN  "<div style='margin-left: 5mm; margin-right: 5mm;'>"
#define HTML_DOC_ITEM_CLOSE "</div>"

#define HTML_EXAMPLE_CODE_OPEN  "<div style='"HTML_CODE_STYLE HTML_EXAMPLE_CODE_STYLE"'>"
#define HTML_EXAMPLE_CODE_CLOSE "</div>"

struct Section_Counter{
    i32 counter[16];
    i32 nest_level;
};

internal b32
doc_get_link_string(Abstract_Item *doc, char *space, i32 capacity){
    String str = make_string_cap(space, 0, capacity);
    append(&str, doc->name);
    append(&str, ".html");
    b32 result = terminate_with_null(&str);
    return(result);
}

internal b32
img_get_link_string(Abstract_Item *img, char *space, i32 capacity, i32 w, i32 h){
    String str = make_string_cap(space, 0, capacity);
    append(&str, img->name);
    
    append(&str, "_");
    append_int_to_str(&str, w);
    append(&str, "_");
    append_int_to_str(&str, h);
    
    append(&str, ".");
    append(&str, img->extension);
    b32 result = terminate_with_null(&str);
    return(result);
}

internal void
append_section_number_reduced(String *out, Section_Counter *section_counter, i32 reduce){
    i32 level = section_counter->nest_level-reduce;
    for (i32 i = 1; i <= level; ++i){
        append_int_to_str(out, section_counter->counter[i]);
        if (i != level){
            append(out, ".");
        }
    }
}

internal void
append_section_number(String *out, Section_Counter *section_counter){
    append_section_number_reduced(out, section_counter, 0);
}

internal i32
extract_command_body(String *out, String l, i32 *i_in_out, i32 *body_start_out, i32 *body_end_out, String command_name, i32 require_body){
    i32 result = 0;
    
    i32 i = *i_in_out;
    
    for (; i < l.size; ++i){
        if (!char_is_whitespace(l.str[i])){
            break;
        }
    }
    
    i32 found_command_body = 0;
    i32 body_start = 0, body_end = 0;
    if (l.str[i] == '{'){
        body_start = i+1;
        
        for (++i; i < l.size; ++i){
            if (l.str[i] == '}'){
                found_command_body = 1;
                body_end = i;
                ++i;
                break;
            }
        }
    }
    
    if (found_command_body){
        result = 1;
    }
    else{
        if (require_body){
#define STR_START "<span style='color:#F00'>! Doc generator error: missing body for "
#define STR_SLOW " !</span>"
            append(out, STR_START);
            append(out, command_name);
            append(out, STR_SLOW);
#undef STR
            fprintf(stderr, "error: missing body for %.*s\n", command_name.size, command_name.str);
        }
    }
    
    *i_in_out = i;
    *body_start_out = body_start;
    *body_end_out = body_end;
    
    return(result);
}

internal void
html_render_section_header(String *out, String section_name, String section_id, Section_Counter *section_counter){
    if (section_counter->nest_level <= 1){
        if (section_id.size > 0){
            append(out, "\n<h2 id='section_");
            append(out, section_id);
            append(out, "'>&sect;");
        }
        else{
            append(out, "\n<h2>&sect;");
        }
        append_section_number(out, section_counter);
        append(out, " ");
        append(out, section_name);
        append(out, "</h2>");
    }
    else{
        if (section_id.size > 0){
            append(out, "<h3 id='section_");
            append(out, section_id);
            append(out, "'>&sect;");
        }
        else{
            append(out, "<h3>&sect;");
        }
        append_section_number(out, section_counter);
        append(out, " ");
        append(out, section_name);
        append(out, "</h3>");
    }
}

#define HTML_WIDTH 800

internal void
write_enriched_text_html(String *out, Enriched_Text *text, Document_System *doc_system,  Section_Counter *section_counter){
    String source = text->source;
    
    append(out, "<div>");
    
    i32 item_counter = 0;
    
    for (String line = get_first_double_line(source);
         line.str;
         line = get_next_double_line(source, line)){
        String l  = skip_chop_whitespace(line);
        append(out, "<p>");
        
        i32 start = 0, i = 0;
        for (; i < l.size; ++i){
            char ch = l.str[i];
            switch (ch){
                case '<':
                {
                    append(out, substr(l, start, i-start));
                    append(out, "&lt;");
                    start = i+1;
                }break;
                
                case '>':
                {
                    append(out, substr(l, start, i-start));
                    append(out, "&gt;");
                    start = i+1;
                }break;
                
                case '\\':
                {
                    append(out, substr(l, start, i-start));
                    
                    i32 command_start = i+1;
                    i32 command_end = command_start;
                    for (; command_end < l.size; ++command_end){
                        if (!char_is_alpha_numeric(l.str[command_end])){
                            break;
                        }
                    }
                    
                    if (command_end == command_start){
                        if (command_end < l.size && l.str[command_end] == '\\'){
                            ++command_end;
                        }
                    }
                    
                    String command_string = substr(l, command_start, command_end - command_start);
                    
                    enum Command_Types{
                        Cmd_BackSlash,
                        Cmd_Version,
                        Cmd_BeginStyle,
                        Cmd_EndStyle,
                        Cmd_DocLink,
                        Cmd_BeginList,
                        Cmd_EndList,
                        Cmd_BeginItem,
                        Cmd_EndItem,
                        Cmd_BoldFace,
                        Cmd_Section,
                        Cmd_BeginLink,
                        Cmd_EndLink,
                        Cmd_Image,
                        Cmd_Video,
                        // never below this
                        Cmd_COUNT,
                    };
                    
                    local_persist String enriched_commands[Cmd_COUNT];
                    
                    enriched_commands[Cmd_BackSlash]  = make_lit_string("\\");
                    enriched_commands[Cmd_Version]    = make_lit_string("VERSION");
                    enriched_commands[Cmd_BeginStyle] = make_lit_string("BEGIN_STYLE");
                    enriched_commands[Cmd_EndStyle]   = make_lit_string("END_STYLE");
                    enriched_commands[Cmd_DocLink]    = make_lit_string("DOC_LINK");
                    enriched_commands[Cmd_BeginList]  = make_lit_string("BEGIN_LIST");
                    enriched_commands[Cmd_EndList]    = make_lit_string("END_LIST");
                    enriched_commands[Cmd_BeginItem]  = make_lit_string("BEGIN_ITEM");
                    enriched_commands[Cmd_EndItem]    = make_lit_string("END_ITEM");
                    enriched_commands[Cmd_BoldFace]   = make_lit_string("BOLD_FACE");
                    enriched_commands[Cmd_Section]    = make_lit_string("SECTION");
                    enriched_commands[Cmd_BeginLink]  = make_lit_string("BEGIN_LINK");
                    enriched_commands[Cmd_EndLink]    = make_lit_string("END_LINK");
                    enriched_commands[Cmd_Image]      = make_lit_string("IMAGE");
                    enriched_commands[Cmd_Video]      = make_lit_string("VIDEO");
                    
                    i = command_end;
                    
                    i32 match_index = 0;
                    if (string_set_match(enriched_commands, ArrayCount(enriched_commands), command_string, &match_index)){
                        switch (match_index){
                            case Cmd_BackSlash: append(out, "\\"); break;
                            case Cmd_Version: append(out, VERSION); break;
                            
                            case Cmd_BeginStyle:
                            {
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    if (match_sc(body_text, "code")){
                                        append(out, "<span style='"HTML_CODE_STYLE"'>");
                                    }
                                }
                            }break;
                            
                            case Cmd_EndStyle:
                            {
                                append(out, "</span>");
                            }break;
                            
                            // TODO(allen): upgrade this bs
                            case Cmd_DocLink:
                            {
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    append(out, "<a href='#");
                                    append(out, body_text);
                                    append(out, "_doc'>");
                                    append(out, body_text);
                                    append(out, "</a>");
                                }
                            }break;
                            
                            case Cmd_BeginList:
                            {
                                append(out,"<ul style='margin-top: 5mm; margin-left: 1mm;'>");
                            }break;
                            
                            case Cmd_EndList:
                            {
                                append(out, "</ul>");
                            }break;
                            
                            case Cmd_BeginItem:
                            {
                                if (item_counter == 0){
                                    append(out, "<li style='font-size: 95%; background: #EFEFDF;'>");
                                    ++item_counter;
                                }
                                else{
                                    append(out, "<li style='font-size: 95%;'>");
                                    item_counter = 0;
                                }
                            }break;
                            
                            case Cmd_EndItem:
                            {
                                append(out, "</li>");
                            }break;
                            
                            case Cmd_Section:
                            {
                                // TODO(allen): undo the duplication of this body extraction code.
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    
                                    html_render_section_header(out, body_text, null_string, section_counter);
                                    ++section_counter->counter[section_counter->nest_level];
                                    item_counter = 0;
                                }
                            }break;
                            
                            case Cmd_BeginLink:
                            {
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    
                                    append(out, "<a ");
                                    if (body_text.str[0] == '!'){
                                        append(out, "target='_blank' ");
                                        body_text.str++;
                                        body_text.size--;
                                    }
                                    append(out, "href='");
                                    if (match_part_sc(body_text, "document:")){
                                        String doc_name = substr_tail(body_text, sizeof("document:")-1);
                                        Abstract_Item *doc_lookup = get_item_by_name(doc_system->doc_list, doc_name);
                                        if (doc_lookup){
                                            char space[256];
                                            if (doc_get_link_string(doc_lookup, space, sizeof(space))){
                                                append(out, space);
                                            }
                                            else{
                                                NotImplemented;
                                            }
                                        }
                                    }
                                    else{
                                        append(out, body_text);
                                    }
                                    append(out, "'>");
                                }
                            }break;
                            
                            case Cmd_EndLink:
                            {
                                append(out, "</a>");
                            }break;
                            
                            case Cmd_Image:
                            {
                                // TODO(allen): generalize this
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    
                                    i32 pixel_height = 10;
                                    i32 pixel_width = HTML_WIDTH;
                                    
                                    body_start = 0, body_end = 0;
                                    has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 0);
                                    if (has_body){
                                        String size_parameter = substr(l, body_start, body_end - body_start);
                                        if (match_part_sc(size_parameter, "width:")){
                                            String width_string = substr_tail(size_parameter, sizeof("width:")-1);
                                            if (str_is_int_s(width_string)){
                                                pixel_width = str_to_int_s(width_string);
                                            }
                                        }
                                    }
                                    
                                    if (match_part_sc(body_text, "image:")){
                                        String img_name = substr_tail(body_text, sizeof("image:")-1);
                                        Abstract_Item *img_lookup = get_item_by_name(doc_system->img_list, img_name);
                                        
                                        if (img_lookup){
                                            pixel_height = ceil32(pixel_width*img_lookup->h_w_ratio);
                                            
                                            append(out, "<img src='");
                                            
                                            char space[256];
                                            if (img_get_link_string(img_lookup, space, sizeof(space), pixel_width, pixel_height)){
                                                append(out, space);
                                                add_image_instantiation(&img_lookup->img_instantiations, pixel_width, pixel_height);
                                            }
                                            else{
                                                NotImplemented;
                                            }
                                            
                                            append(out, "' style='width: ");
                                            append_int_to_str(out, pixel_width);
                                            append(out, "px; height: ");
                                            append_int_to_str(out, pixel_height);
                                            append(out, "px;'>");
                                        }
                                    }
                                }
                            }break;
                            
                            case Cmd_Video:
                            {
                                // TODO(allen): generalize this
                                i32 body_start = 0, body_end = 0;
                                i32 has_body = extract_command_body(out, l, &i, &body_start, &body_end, command_string, 1);
                                
                                i32 pixel_width = HTML_WIDTH;
                                i32 pixel_height = (i32)(pixel_width * 0.5625);
                                
                                if (has_body){
                                    String body_text = substr(l, body_start, body_end - body_start);
                                    body_text = skip_chop_whitespace(body_text);
                                    
                                    if (match_part_sc(body_text, "youtube:")){
                                        String youtube_str = substr_tail(body_text, sizeof("youtube:")-1);
                                        
                                        append(out, "<iframe  width='");
                                        append_int_to_str(out, pixel_width);
                                        append(out, "' height='");
                                        append_int_to_str(out, pixel_height);
                                        append(out, "' src='");
                                        append(out, youtube_str);
                                        append(out, "' allowfullscreen> </iframe>");
                                    }
                                }
                            }break;
                        }
                    }
                    else{
                        append(out, "<span style='color:#F00'>! Doc generator error: unrecognized command !</span>");
                        fprintf(stderr, "error: unrecognized command %.*s\n", command_string.size, command_string.str);
                    }
                    
                    start = i;
                }
            }
        }
        
        if (start != i){
            append(out, substr(l, start, i-start));
        }
        
        append(out, "</p>");
    }
    
    append(out, "</div>");
}

internal void
print_item_in_list(String *out, String name, char *id_postfix){
    append(out, "<li><a href='#");
    append(out, name);
    append(out, id_postfix);
    append(out, "'>");
    append(out, name);
    append(out, "</a></li>");
}

internal void
init_used_links(Used_Links *used, i32 count){
    used->strs = fm_push_array(String, count);
    used->count = 0;
    used->max = count;
}

internal b32
try_to_use_link(Used_Links *used, String str){
    b32 result = true;
    i32 index = 0;
    if (string_set_match(used->strs, used->count, str, &index)){
        result = false;
    }
    else{
        Assert(used->count < used->max);
        used->strs[used->count++] = str;
    }
    return(result);
}

internal void
print_struct_html(String *out, Item_Node *member, i32 hide_children){
    String name = member->name;
    String type = member->type;
    String type_postfix = member->type_postfix;
    
    append     (out, type);
    append_s_char (out, ' ');
    append     (out, name);
    append     (out, type_postfix);
    
    if (match_ss(type, make_lit_string("struct")) ||
        match_ss(type, make_lit_string("union"))){
        
        if (hide_children){
            append(out, " { /* non-public internals */ } ;");
        }
        else{
            append(out, " {<br><div style='margin-left: 8mm;'>");
            
            for (Item_Node *member_iter = member->first_child;
                 member_iter != 0;
                 member_iter = member_iter->next_sibling){
                print_struct_html(out, member_iter, hide_children);
            }
            
            append(out, "</div>};<br>");
        }
    }
    else{
        append(out, ";<br>");
    }
}

internal void
print_function_html(String *out, Used_Links *used, String cpp_name, String ret, char *function_call_head, String name, Argument_Breakdown breakdown){
    
    append     (out, ret);
    append_s_char (out, ' ');
    append     (out, function_call_head);
    append     (out, name);
    
    if (breakdown.count == 0){
        append(out, "()");
    }
    else if (breakdown.count == 1){
        append(out, "(");
        append(out, breakdown.args[0].param_string);
        append(out, ")");
    }
    else{
        append(out, "(<div style='margin-left: 4mm;'>");
        
        for (i32 j = 0; j < breakdown.count; ++j){
            append(out, breakdown.args[j].param_string);
            if (j < breakdown.count - 1){
                append_s_char(out, ',');
            }
            append(out, "<br>");
        }
        
        append(out, "</div>)");
    }
}

internal void
print_macro_html(String *out, String name, Argument_Breakdown breakdown){
    
    append (out, "#define ");
    append (out, name);
    
    if (breakdown.count == 0){
        append(out, "()");
    }
    else if (breakdown.count == 1){
        append_s_char  (out, '(');
        append      (out, breakdown.args[0].param_string);
        append_s_char  (out, ')');
    }
    else{
        append (out, "(<div style='margin-left: 4mm;'>");
        
        for (i32 j = 0; j < breakdown.count; ++j){
            append(out, breakdown.args[j].param_string);
            if (j < breakdown.count - 1){
                append_s_char(out, ',');
            }
            append(out, "<br>");
        }
        
        append(out, ")</div>)");
    }
}

enum Doc_Chunk_Type{
    DocChunk_PlainText,
    DocChunk_CodeExample,
    
    DocChunk_Count
};

global String doc_chunk_headers[] = {
    make_lit_string(""),
    make_lit_string("CODE_EXAMPLE"),
};

internal String
get_next_doc_chunk(String source, String prev_chunk, Doc_Chunk_Type *type){
    String chunk = {0};
    String word = {0};
    i32 pos = source.size;
    i32 word_index = 0;
    Doc_Chunk_Type t = DocChunk_PlainText;
    
    i32 start_pos = (i32)(prev_chunk.str - source.str) + prev_chunk.size;
    String source_tail = substr_tail(source, start_pos);
    
    Assert(DocChunk_Count == ArrayCount(doc_chunk_headers));
    
    for (word = get_first_word(source_tail);
         word.str;
         word = get_next_word(source_tail, word), ++word_index){
        
        for (i32 i = 1; i < DocChunk_Count; ++i){
            if (match_ss(word, doc_chunk_headers[i])){
                pos = (i32)(word.str - source.str);
                t = (Doc_Chunk_Type)i;
                goto doublebreak;
            }
        }
    }
    doublebreak:;
    
    *type = DocChunk_PlainText;
    if (word_index == 0){
        *type = t;
        
        i32 nest_level = 1;
        i32 i = find_s_char(source, pos, '(');
        for (++i; i < source.size; ++i){
            if (source.str[i] == '('){
                ++nest_level;
            }
            else if (source.str[i] == ')'){
                --nest_level;
                if (nest_level == 0){
                    break;
                }
            }
        }
        
        pos = i+1;
    }
    
    chunk = substr(source, start_pos, pos - start_pos);
    
    i32 is_all_white = 1;
    for (i32 i = 0; i < chunk.size; ++i){
        if (!char_is_whitespace(chunk.str[i])){
            is_all_white = 0;
            break;
        }
    }
    
    if (is_all_white){
        chunk = null_string;
    }
    
    return(chunk);
}

internal String
get_first_doc_chunk(String source, Doc_Chunk_Type *type){
    String start_str = make_string(source.str, 0);
    String chunk = get_next_doc_chunk(source, start_str, type);
    return(chunk);
}

internal void
print_doc_description(String *out, String src){
    Doc_Chunk_Type type;
    
    for (String chunk = get_first_doc_chunk(src, &type);
         chunk.str;
         chunk = get_next_doc_chunk(src, chunk, &type)){
        
        switch (type){
            case DocChunk_PlainText:
            {
                for (String line = get_first_double_line(chunk);
                     line.str;
                     line = get_next_double_line(chunk, line)){
                    append(out, line);
                    append(out, "<br><br>");
                }
            }break;
            
            case DocChunk_CodeExample:
            {
                i32 start = 0;
                i32 end = chunk.size-1;
                while (start < end && chunk.str[start] != '(') ++start;
                start += 1;
                while (end > start && chunk.str[end] != ')') --end;
                
                
                append(out, HTML_EXAMPLE_CODE_OPEN);
                
                if (start < end){
                    String code_example = substr(chunk, start, end - start);
                    i32 first_line = 1;
                    
                    for (String line = get_first_line(code_example);
                         line.str;
                         line = get_next_line(code_example, line)){
                        
                        if (!(first_line && line.size == 0)){
                            i32 space_i = 0;
                            for (; space_i < line.size; ++space_i){
                                if (line.str[space_i] == ' '){
                                    append(out, "&nbsp;");
                                }
                                else{
                                    break;
                                }
                            }
                            
                            String line_tail = substr_tail(line, space_i);
                            append(out, line_tail);
                            append(out, "<br>");
                        }
                        first_line = 0;
                    }
                }
                
                append(out, HTML_EXAMPLE_CODE_CLOSE);
            }break;
        }
    }
}

internal void
print_struct_docs(String *out, Item_Node *member){
    for (Item_Node *member_iter = member->first_child;
         member_iter != 0;
         member_iter = member_iter->next_sibling){
        String type = member_iter->type;
        if (match_ss(type, make_lit_string("struct")) ||
            match_ss(type, make_lit_string("union"))){
            print_struct_docs(out, member_iter);
        }
        else{
            Documentation doc = {0};
            perform_doc_parse(member_iter->doc_string, &doc);
            
            append(out, "<div>");
            
            append(out, "<div style='"HTML_CODE_STYLE"'>"HTML_DOC_ITEM_HEAD_INL_OPEN);
            append(out, member_iter->name);
            append(out, HTML_DOC_ITEM_HEAD_INL_CLOSE"</div>");
            
            append(out, "<div style='margin-bottom: 6mm;'>"HTML_DOC_ITEM_OPEN);
            print_doc_description(out, doc.main_doc);
            append(out, HTML_DOC_ITEM_CLOSE"</div>");
            
            append(out, "</div>");
        }
    }
}

internal void
print_see_also(String *out, Documentation *doc){
    i32 doc_see_count = doc->see_also_count;
    if (doc_see_count > 0){
        append(out, HTML_DOC_HEAD_OPEN"See Also"HTML_DOC_HEAD_CLOSE);
        
        for (i32 j = 0; j < doc_see_count; ++j){
            String see_also = doc->see_also[j];
            append(out, HTML_DOC_ITEM_OPEN"<a href='#");
            append(out, see_also);
            append(out, "_doc'>");
            append(out, see_also);
            append(out, "</a>"HTML_DOC_ITEM_CLOSE);
        }
    }
}

internal void
print_function_docs(String *out, String name, String doc_string){
    if (doc_string.size == 0){
        append(out, "No documentation generated for this function.");
        fprintf(stderr, "warning: no documentation string for %.*s\n", name.size, name.str);
    }
    
    Temp temp = fm_begin_temp();
    
    Documentation doc = {0};
    
    perform_doc_parse(doc_string, &doc);
    
    i32 doc_param_count = doc.param_count;
    if (doc_param_count > 0){
        append(out, HTML_DOC_HEAD_OPEN"Parameters"HTML_DOC_HEAD_CLOSE);
        
        for (i32 j = 0; j < doc_param_count; ++j){
            String param_name = doc.param_name[j];
            String param_docs = doc.param_docs[j];
            
            // TODO(allen): check that param_name is actually a parameter to this function!
            
            append(out, "<div>"HTML_DOC_ITEM_HEAD_OPEN);
            append(out, param_name);
            append(out, HTML_DOC_ITEM_HEAD_CLOSE"<div style='margin-bottom: 6mm;'>"HTML_DOC_ITEM_OPEN);
            append(out, param_docs);
            append(out, HTML_DOC_ITEM_CLOSE"</div></div>");
        }
    }
    
    String ret_doc = doc.return_doc;
    if (ret_doc.size != 0){
        append(out, HTML_DOC_HEAD_OPEN"Return"HTML_DOC_HEAD_CLOSE HTML_DOC_ITEM_OPEN);
        append(out, ret_doc);
        append(out, HTML_DOC_ITEM_CLOSE);
    }
    
    String main_doc = doc.main_doc;
    if (main_doc.size != 0){
        append(out, HTML_DOC_HEAD_OPEN"Description"HTML_DOC_HEAD_CLOSE HTML_DOC_ITEM_OPEN);
        print_doc_description(out, main_doc);
        append(out, HTML_DOC_ITEM_CLOSE);
    }
    
    print_see_also(out, &doc);
    
    fm_end_temp(temp);
}

internal void
print_item_html(String *out, Used_Links *used, Item_Node *item, char *id_postfix, char *section, i32 I, Alternate_Name *alt_name, i32 alt_name_type){
    Temp temp = fm_begin_temp();
    
    String name = item->name;
    
    switch (alt_name_type){
        case AltName_Macro:
        {
            name = alt_name->macro;
        }break;
        
        case AltName_Public_Name:
        {
            name = alt_name->public_name;
        }break;
    }
    
    /* NOTE(allen):
    Open a div for the whole item.
    Put a heading in it with the name and section.
    Open a "descriptive" box for the display of the code interface.
    */
    append(out, "<div id='");
    append(out, name);
    append(out, id_postfix);
    append(out, "' style='margin-bottom: 1cm;'>");
    
    i32 has_cpp_name = 0;
    if (item->cpp_name.str != 0){
        if (try_to_use_link(used, item->cpp_name)){
            append(out, "<div id='");
            append(out, item->cpp_name);
            append(out, id_postfix);
            append(out, "'>");
            has_cpp_name = 1;
        }
    }
    
    append         (out, "<h4>&sect;");
    append         (out, section);
    append_s_char     (out, '.');
    append_int_to_str (out, I);
    append         (out, ": ");
    append         (out, name);
    append         (out, "</h4>");
    
    append(out, "<div style='"HTML_CODE_STYLE" "HTML_DESCRIPT_SECTION_STYLE"'>");
    
    switch (item->t){
        case Item_Function:
        {
            // NOTE(allen): Code box
            print_function_html(out, used, item->cpp_name, item->ret, "", name, item->breakdown);
            
            // NOTE(allen): Close the code box
            append(out, "</div>");
            
            // NOTE(allen): Descriptive section
            print_function_docs(out, name, item->doc_string);
        }break;
        
        case Item_Macro:
        {
            // NOTE(allen): Code box
            print_macro_html(out, name, item->breakdown);
            
            // NOTE(allen): Close the code box
            append(out, "</div>");
            
            // NOTE(allen): Descriptive section
            print_function_docs(out, name, item->doc_string);
        }break;
        
        case Item_Typedef:
        {
            String type = item->type;
            
            // NOTE(allen): Code box
            append     (out, "typedef ");
            append     (out, type);
            append_s_char (out, ' ');
            append     (out, name);
            append_s_char (out, ';');
            
            // NOTE(allen): Close the code box
            append(out, "</div>");
            
            // NOTE(allen): Descriptive section
            String doc_string = item->doc_string;
            Documentation doc = {0};
            perform_doc_parse(doc_string, &doc);
            
            String main_doc = doc.main_doc;
            if (main_doc.size != 0){
                append(out, HTML_DOC_HEAD_OPEN"Description"HTML_DOC_HEAD_CLOSE);
                
                append(out, HTML_DOC_ITEM_OPEN);
                print_doc_description(out, main_doc);
                append(out, HTML_DOC_ITEM_CLOSE);
            }
            else{
                fprintf(stderr, "warning: no documentation string for %.*s\n", name.size, name.str);
            }
            
            print_see_also(out, &doc);
            
        }break;
        
        case Item_Enum:
        {
            // NOTE(allen): Code box
            append     (out, "enum ");
            append     (out, name);
            append_s_char (out, ';');
            
            // NOTE(allen): Close the code box
            append(out, "</div>");
            
            // NOTE(allen): Descriptive section
            String doc_string = item->doc_string;
            Documentation doc = {0};
            perform_doc_parse(doc_string, &doc);
            
            String main_doc = doc.main_doc;
            if (main_doc.size != 0){
                append(out, HTML_DOC_HEAD_OPEN"Description"HTML_DOC_HEAD_CLOSE);
                
                append(out, HTML_DOC_ITEM_OPEN);
                print_doc_description(out, main_doc);
                append(out, HTML_DOC_ITEM_CLOSE);
            }
            else{
                fprintf(stderr, "warning: no documentation string for %.*s\n", name.size, name.str);
            }
            
            if (item->first_child){
                append(out, HTML_DOC_HEAD_OPEN"Values"HTML_DOC_HEAD_CLOSE);
                
                for (Item_Node *member = item->first_child;
                     member;
                     member = member->next_sibling){
                    Documentation doc = {0};
                    perform_doc_parse(member->doc_string, &doc);
                    
                    append(out, "<div>");
                    
                    // NOTE(allen): Dafuq is this all?
                    append(out, "<div><span style='"HTML_CODE_STYLE"'>"HTML_DOC_ITEM_HEAD_INL_OPEN);
                    append(out, member->name);
                    append(out, HTML_DOC_ITEM_HEAD_INL_CLOSE);
                    
                    if (member->value.str){
                        append(out, " = ");
                        append(out, member->value);
                    }
                    
                    append(out, "</span></div>");
                    
                    append(out, "<div style='margin-bottom: 6mm;'>"HTML_DOC_ITEM_OPEN);
                    print_doc_description(out, doc.main_doc);
                    append(out, HTML_DOC_ITEM_CLOSE"</div>");
                    
                    append(out, "</div>");
                }
            }
            
            print_see_also(out, &doc);
            
        }break;
        
        case Item_Struct: case Item_Union:
        {
            String doc_string = item->doc_string;
            
            i32 hide_members = 0;
            
            if (doc_string.size == 0){
                hide_members = 1;
            }
            else{
                for (String word = get_first_word(doc_string);
                     word.str;
                     word = get_next_word(doc_string, word)){
                    if (match_ss(word, make_lit_string("HIDE_MEMBERS"))){
                        hide_members = 1;
                        break;
                    }
                }
            }
            
            // NOTE(allen): Code box
            print_struct_html(out, item, hide_members);
            
            // NOTE(allen): Close the code box
            append(out, "</div>");
            
            // NOTE(allen): Descriptive section
            {
                Documentation doc = {0};
                perform_doc_parse(doc_string, &doc);
                
                String main_doc = doc.main_doc;
                if (main_doc.size != 0){
                    append(out, HTML_DOC_HEAD_OPEN"Description"HTML_DOC_HEAD_CLOSE);
                    
                    append(out, HTML_DOC_ITEM_OPEN);
                    print_doc_description(out, main_doc);
                    append(out, HTML_DOC_ITEM_CLOSE);
                }
                else{
                    fprintf(stderr, "warning: no documentation string for %.*s\n", name.size, name.str);
                }
                
                if (!hide_members){
                    if (item->first_child){
                        append(out, HTML_DOC_HEAD_OPEN"Fields"HTML_DOC_HEAD_CLOSE);
                        print_struct_docs(out, item);
                    }
                }
                
                print_see_also(out, &doc);
            }
        }break;
    }
    
    if (has_cpp_name){
        append(out, "</div>");
    }
    
    // NOTE(allen): Close the item box
    append(out, "</div><hr>");
    
    fm_end_temp(temp);
}

global char* html_css =
"<style>"
"body { "
"background: " HTML_BACK_COLOR "; "
"color: " HTML_TEXT_COLOR "; "
"}"

// H things
"h1,h2,h3,h4 { "
"color: " HTML_POP_COLOR_1 "; "
"margin: 0; "
"}"

"h2 { "
"margin-top: 6mm; "
"}"

"h3 { "
"margin-top: 5mm; margin-bottom: 5mm; "
"}"

"h4 { "
"font-size: 1.1em; "
"}"

// ANCHORS
"a { "
"color: " HTML_POP_COLOR_1 "; "
"text-decoration: none; "
"}"
"a:visited { "
"color: " HTML_VISITED_LINK "; "
"}"
"a:hover { "
"background: " HTML_POP_BACK_1 "; "
"}"

// LIST
"ul { "
"list-style: none; "
"padding: 0; "
"margin: 0; "
"}"
"</style>";

internal void
doc_item_html(String *out, Document_System *doc_system, Used_Links *used_links, Document_Item *item, Section_Counter *section_counter, b32 head){
    switch (item->type){
        case Doc_Root:
        {
            if (head){
                append(out,
                       "<html lang=\"en-US\">"
                       "<head>"
                       "<link rel='shortcut icon' type='image/x-icon' href='4coder_icon.ico' />"
                       "<title>");
                append(out, item->section.name);
                append(out, "</title>");
                
                append(out, html_css);
                
                append(out,
                       "</head>\n"
                       "<body>"
                       "<div style='font-family:Arial; margin: 0 auto; "
                       "width: ");
                append_int_to_str(out, HTML_WIDTH);
                append(out, "px; text-align: justify; line-height: 1.25;'>");
                
                if (item->section.show_title){
                    append(out, "<h1 style='margin-top: 5mm; margin-bottom: 5mm;'>");
                    append(out, item->section.name);
                    append(out, "</h1>");
                }
            }
            else{
                append(out, "</div></body></html>");
            }
        }break;
        
        case Doc_Section:
        {
            if (head){
                html_render_section_header(out, item->section.name, item->section.id, section_counter);
            }
        }break;
        
        case Doc_Todo:
        {
            if (head){
                append(out, "<div><i>Coming Soon</i><div>");
            }
        }break;
        
        case Doc_Enriched_Text:
        {
            if (head){
                write_enriched_text_html(out, item->text.text, doc_system, section_counter);
            }
        }break;
        
        case Doc_Element_List:
        {
            if (head){
                append(out, "<ul>");
                
                Meta_Unit *unit = item->unit_elements.unit;
                Alternate_Names_Array *alt_names = item->unit_elements.alt_names;
                i32 count = unit->set.count;
                
                switch (item->unit_elements.alt_name_type){
                    case AltName_Standard:
                    {
                        for (i32 i = 0; i < count; ++i){
                            print_item_in_list(out, unit->set.items[i].name, "_doc");
                        }
                    }break;
                    
                    case AltName_Macro:
                    {
                        for (i32 i = 0; i < count; ++i){
                            print_item_in_list(out, alt_names->names[i].macro, "_doc");
                        }
                    }break;
                    
                    case AltName_Public_Name:
                    {
                        for (i32 i = 0; i < count; ++i){
                            print_item_in_list(out, alt_names->names[i].public_name, "_doc");
                        }
                    }break;
                }
                
                append(out, "</ul>");
            }
        }break;
        
        case Doc_Full_Elements:
        {
            if (head){
                Meta_Unit *unit = item->unit_elements.unit;
                Alternate_Names_Array *alt_names = item->unit_elements.alt_names;
                i32 count = unit->set.count;
                
                char section_space[32];
                String section_str = make_fixed_width_string(section_space);
                append_section_number_reduced(&section_str, section_counter, 1);
                terminate_with_null(&section_str);
                
                if (alt_names){
                    i32 I = 1;
                    for (i32 i = 0; i < count; ++i, ++I){
                        print_item_html(out, used_links, &unit->set.items[i], "_doc", section_str.str, I, &alt_names->names[i], item->unit_elements.alt_name_type);
                    }
                }
                else{
                    i32 I = 1;
                    for (i32 i = 0; i < count; ++i, ++I){
                        print_item_html(out, used_links, &unit->set.items[i], "_doc", section_str.str, I, 0, 0);
                    }
                }
            }
        }break;
        
        case Doc_Table_Of_Contents:
        {
            if (head){
                append(out, "<h3 style='margin:0;'>Table of Contents</h3><ul>");
                
                i32 i = 1;
                for (Document_Item *toc_item = item->parent->section.first_child;
                     toc_item != 0;
                     toc_item = toc_item->next){
                    if (toc_item->type == Doc_Section){
                        if (toc_item->section.id.size > 0){
                            append(out, "<li><a href='#section_");
                            append(out, toc_item->section.id);
                            append(out, "'>&sect;");
                        }
                        else{
                            append(out, "<li>&sect;");
                        }
                        append_int_to_str (out, i);
                        append_s_char     (out, ' ');
                        append            (out, toc_item->section.name);
                        append            (out, "</a></li>");
                        ++i;
                    }
                }
                
                append(out, "</ul>");
            }
        }break;
    }
}

internal void
generate_item_html(String *out, Document_System *doc_system, Used_Links *used_links, Document_Item *item, Section_Counter *section_counter){
    doc_item_html(out, doc_system, used_links, item, section_counter, true);
    
    if (item->type == Doc_Root || item->type == Doc_Section){
        i32 level = ++section_counter->nest_level;
        section_counter->counter[level] = 1;
        for (Document_Item *m = item->section.first_child;
             m != 0;
             m = m->next){
            generate_item_html(out, doc_system, used_links, m, section_counter);
        }
        --section_counter->nest_level;
        ++section_counter->counter[section_counter->nest_level];
    }
    
    doc_item_html(out, doc_system, used_links, item, section_counter, false);
}

internal void
generate_document_html(String *out, Document_System *doc_system, Abstract_Item *doc){
    Assert(doc->root_item != 0);
    
    Used_Links used_links = {0};
    init_used_links(&used_links, 4000);
    
    Section_Counter section_counter = {0};
    section_counter.counter[section_counter.nest_level] = 1;
    generate_item_html(out, doc_system, &used_links, doc->root_item, &section_counter);
}

// BOTTOM
