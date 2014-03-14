/* ***************************************************************************
 * label.c -- LCUI's Label widget
 * 
 * Copyright (C) 2012-2014 by
 * Liu Chao
 * 
 * This file is part of the LCUI project, and may only be used, modified, and
 * distributed under the terms of the GPLv2.
 * 
 * (GPLv2 is abbreviation of GNU General Public License Version 2)
 * 
 * By continuing to use, modify, or distribute this file you indicate that you
 * have read the license and understand and accept it fully.
 *  
 * The LCUI project is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GPL v2 for more details.
 * 
 * You should have received a copy of the GPLv2 along with this file. It is 
 * usually in the LICENSE.TXT file, If not, see <http://www.gnu.org/licenses/>.
 * ****************************************************************************/
 
/* ****************************************************************************
 * label.c -- LCUI 的文本标签部件
 *
 * 版权所有 (C) 2012-2014 归属于
 * 刘超
 * 
 * 这个文件是LCUI项目的一部分，并且只可以根据GPLv2许可协议来使用、更改和发布。
 *
 * (GPLv2 是 GNU通用公共许可证第二版 的英文缩写)
 * 
 * 继续使用、修改或发布本文件，表明您已经阅读并完全理解和接受这个许可协议。
 * 
 * LCUI 项目是基于使用目的而加以散布的，但不负任何担保责任，甚至没有适销性或特
 * 定用途的隐含担保，详情请参照GPLv2许可协议。
 *
 * 您应已收到附随于本文件的GPLv2许可协议的副本，它通常在LICENSE.TXT文件中，如果
 * 没有，请查看：<http://www.gnu.org/licenses/>. 
 * ****************************************************************************/

//#define DEBUG

#include <LCUI_Build.h>
#include LC_LCUI_H
#include LC_WIDGET_H
#include LC_LABEL_H

enum label_msg_id {
	LABEL_TEXT = WIDGET_USER+1,
	LABEL_REFRESH,
	LABEL_UPDATE_SIZE,
	LABEL_AUTO_WRAP,
	LABEL_STYLE
};

typedef struct LCUI_Label_ {
	LCUI_BOOL auto_size;	/* 指定是否根据文本图层的尺寸来调整部件尺寸 */
	AUTOSIZE_MODE mode;	/* 自动尺寸调整的模式 */
	LCUI_TextLayer layer;	/* 文本图层 */
}
LCUI_Label;

/*---------------------------- Private -------------------------------*/

static void Label_UpdateTextLayer( LCUI_Widget *widget )
{
	int n; 
	LCUI_Label *label;
	LCUI_Rect *p_rect;
	LCUI_Queue rect_list;

	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}

	Queue_Init( &rect_list, sizeof(LCUI_Rect), NULL );
	/* 先更新文本图层的数据 */
	TextLayer_Update( &label->layer, &rect_list );
	n = Queue_GetTotal( &rect_list );
	/* 将得到的无效区域导入至部件的无效区域列表 */
	while(n--) {
		p_rect = (LCUI_Rect*)Queue_Get( &rect_list, n );
		if( !p_rect ) {
			continue;
		}
		Widget_InvalidArea( widget, *p_rect );
	}
	Queue_Destroy( &rect_list );
	TextLayer_ClearInvalidRect( &label->layer );
}

/** 更新TextLayer的尺寸 */
static void Label_UpdateTextLayerSize( LCUI_Widget *widget, void *arg )
{
	LCUI_Size new_size;
	LCUI_Label *label;

	if( !widget ) {
		return;
	}
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}

	Widget_Lock( widget );
	new_size.w = TextLayer_GetWidth( &label->layer );
	new_size.h = TextLayer_GetHeight( &label->layer );
	/* 如果部件尺寸不是由LCUI自动调整的 */
	if( widget->dock != DOCK_TYPE_NONE || !label->auto_size ) {
		new_size = Widget_GetSize(widget);
		/* 如果部件尺寸不合TextLayer尺寸一致 */
		if( label->layer.graph.w != new_size.w
		 || label->layer.graph.h != new_size.h ) {
			Widget_Draw( widget ); 
		}
		if( new_size.w < 20 ) {
			new_size.w = 20;
		}
		if( new_size.h < 20 ) {
			new_size.h = 20;
		}
		TextLayer_SetMaxSize( &label->layer, new_size );
		Widget_Unlock( widget );
		return;
	}
	/* 如果未启用自动换行 */
	if( !label->layer.is_autowrap_mode ) {
		if( new_size.w != widget->size.w
		 || new_size.h != widget->size.h ) {
			TextLayer_SetMaxSize( &label->layer, new_size );
			Widget_Resize( widget, new_size );
		}
		Widget_Unlock( widget );
		return;
	}
	
	new_size = Widget_GetSize(widget);
	/* 将部件所在容器的宽度作为图像宽度 */
	new_size.w = Widget_GetContainerWidth( widget->parent );
	/* 高度必须有效，最小高度为20 */
	if( new_size.h < 20 ) {
		new_size.h = 20;
	}
	/* 如果部件尺寸等于TextLayer的图像尺寸 */
	if( label->layer.graph.w == new_size.w
	 && label->layer.graph.h == new_size.h ) {
		Widget_Unlock( widget );
		return;
	}
	/* 重新设置最大尺寸 */
	TextLayer_SetMaxSize( &label->layer, new_size );
	Label_UpdateTextLayer( widget );
	/* 重新计算文本图层的尺寸 */
	new_size.w = TextLayer_GetWidth( &label->layer );
	new_size.h = TextLayer_GetHeight( &label->layer );
	TextLayer_SetMaxSize( &label->layer, new_size );
	Widget_Resize( widget, new_size );
	Widget_Unlock( widget );
}

/** 刷新Label部件的文本图层 */
static void Label_RefreshTextLayer( LCUI_Widget *widget, void *arg )
{
	LCUI_Label *label;

	if( !widget ) {
		return;
	}
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}

	Widget_Lock( widget );
	Label_UpdateTextLayer( widget );
	Widget_Update( widget );
	Widget_Draw( widget );
	Widget_Unlock( widget );
}

static void Label_SetTextStyle( LCUI_Widget *widget, void *arg )
{
	LCUI_TextStyle *style;
	LCUI_Label *label;
	
	DEBUG_MSG("recv LABEL_STYLE msg, lock widget\n");
	style = (LCUI_TextStyle*)arg;
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}
	Widget_Lock( widget );
	TextLayer_SetTextStyle( &label->layer, style );
	Widget_Update( widget );
	DEBUG_MSG("unlock widget\n");
	Widget_Unlock( widget );
}

static void Label_SetTextW( LCUI_Widget *widget, void *arg )
{
	LCUI_Label *label;
	DEBUG_MSG("recv LABEL_TEXT msg, lock widget\n");
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}
	Widget_Lock( widget );
	TextLayer_SetTextW( &label->layer, (wchar_t*)arg, NULL );
	Label_UpdateTextLayer( widget );
	Label_UpdateTextLayerSize( widget, NULL );
	DEBUG_MSG("unlock widget\n");
	Widget_Unlock( widget );
}

static void Label_ExecSetAutoWrap( LCUI_Widget *widget, void *arg )
{
	LCUI_BOOL flag;
	LCUI_Label *label;

	flag = arg?TRUE:FALSE;
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}
	Widget_Lock( widget );
	TextLayer_SetAutoWrap( &label->layer, flag );
	Widget_Draw( widget );
	Widget_Unlock( widget );
}

/** 初始化label部件数据 */
static void Label_ExecInit( LCUI_Widget *widget )
{
	LCUI_Label *label;
	
	/* label部件不需要焦点 */
	widget->focus = FALSE;

	label = (LCUI_Label*)Widget_NewPrivData( widget, sizeof(LCUI_Label) );
	label->auto_size = TRUE;
	/* 初始化文本图层 */
	TextLayer_Init( &label->layer ); 
	/* 启用多行文本显示 */
	TextLayer_SetMultiline( &label->layer, TRUE );
	Widget_SetAutoSize( widget, FALSE, 0 );
	/* 启用样式标签的支持 */
	TextLayer_SetUsingStyleTags( &label->layer, TRUE );
	/* 将回调函数与自定义消息关联 */
	WidgetMsg_Connect( widget, LABEL_TEXT, Label_SetTextW );
	WidgetMsg_Connect( widget, LABEL_STYLE, Label_SetTextStyle );
	WidgetMsg_Connect( widget, LABEL_UPDATE_SIZE, Label_UpdateTextLayerSize );
	WidgetMsg_Connect( widget, LABEL_REFRESH, Label_RefreshTextLayer );
	WidgetMsg_Connect( widget, LABEL_AUTO_WRAP, Label_ExecSetAutoWrap );
}

/** 释放label部件占用的资源 */
static void Label_ExecDestroy( LCUI_Widget *widget )
{
	LCUI_Label *label;
	
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	TextLayer_Destroy( &label->layer );
}

/** 更新Label部件的数据 */
static void Label_ExecUpdate( LCUI_Widget *widget )
{
	WidgetMsg_Post( widget, LABEL_UPDATE_SIZE, NULL, TRUE, FALSE );
}

/** 绘制label部件 */
static void Label_ExecDraw( LCUI_Widget *widget )
{
	LCUI_Label *label;
	LCUI_Graph *widget_graph, *tlayer_graph;

	if( !widget ) {
		return;
	}
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	if( !label ) {
		return;
	}
	Label_UpdateTextLayer( widget );
	TextLayer_Draw( &label->layer );
	widget_graph = Widget_GetSelfGraph( widget );
	tlayer_graph = TextLayer_GetGraphBuffer( &label->layer );
	/* 如果部件使用透明背景 */
	if( widget->background.transparent ) {
		Graph_Replace( widget_graph, tlayer_graph, Pos(0,0) );
	} else {
		Graph_Mix( widget_graph, tlayer_graph, Pos(0,0) );
	}
}

/*-------------------------- End Private -----------------------------*/

/*---------------------------- Public --------------------------------*/
/** 设定与标签关联的文本内容 */
LCUI_API int Label_TextW( LCUI_Widget *widget, const wchar_t *text )
{
	int len;
	wchar_t *p_text;

	if( !widget ) {
		return -1;
	}
	if( !text ) {
		len = 0;
	} else {
		len = wcslen( text );
	}
	p_text = (wchar_t*)malloc( sizeof(wchar_t)*(len+1) );
	if( !p_text ) {
		return -1;
	}
	if( !text ) {
		p_text[0] = '\0';
	} else {
		wcscpy( p_text, text );
	}
	DEBUG_MSG("post LABEL_TEXT msg\n");
	WidgetMsg_Post( widget, LABEL_TEXT, p_text, TRUE, TRUE );
	return 0;
}

LCUI_API int Label_Text( LCUI_Widget *widget, const char *utf8_text )
{
	int ret;
	wchar_t *wstr;

	LCUICharset_UTF8ToUnicode( utf8_text, &wstr );
	ret = Label_TextW( widget, wstr );
	if( wstr ) {
		free( wstr );
	}
	return ret;
}

LCUI_API int Label_TextA( LCUI_Widget *widget, const char *ansi_text )
{
	int ret;
	wchar_t *wstr;
	LCUICharset_GB2312ToUnicode( ansi_text, &wstr );
	Label_TextW( widget, wstr );
	if( wstr ) {
		free( wstr );
	}
	return ret;
}

/** 设置Label部件显示的文本是否自动换行 */
LCUI_API void Label_SetAutoWrap( LCUI_Widget *widget, LCUI_BOOL flag )
{
	WidgetMsg_Post( widget, LABEL_AUTO_WRAP, flag?((void*)(1)):NULL, TRUE, FALSE );
}

/** 设置文本对齐方式 */
LCUI_API void Label_SetTextAlign( LCUI_Widget *widget, TextAlignType align )
{
	LCUI_TextLayer *layer;
	layer = Label_GetTextLayer( widget );
	TextLayer_SetTextAlign( layer, align );
	Widget_Update( widget );
}

/** 为Label部件内显示的文本设定文本样式 */
LCUI_API int Label_TextStyle( LCUI_Widget *widget, LCUI_TextStyle style )
{
	LCUI_TextStyle *p_style;
	
	if( !widget ) {
		return -1;
	}
	p_style = (LCUI_TextStyle*)malloc( sizeof(LCUI_TextStyle) );
	if( !p_style ) {
		return -2;
	}
	*p_style = style;
	DEBUG_MSG("post LABEL_STYLE msg\n");
	WidgetMsg_Post( widget, LABEL_STYLE, p_style, TRUE, TRUE );
	return 0;
}

/** 获取Label部件的文本样式 */
LCUI_API LCUI_TextStyle Label_GetTextStyle( LCUI_Widget *widget )
{
	LCUI_TextLayer *layer;
	layer = Label_GetTextLayer( widget );
	return layer->text_style;
}

/** 获取label部件内的文本图层的指针 */
LCUI_API LCUI_TextLayer* Label_GetTextLayer( LCUI_Widget *widget )
{
	LCUI_Label *label;
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	return &label->layer;
}

/** 刷新label部件显示的文本 */
LCUI_API void Label_Refresh( LCUI_Widget *widget )
{
	WidgetMsg_Post( widget, LABEL_REFRESH, NULL, TRUE, FALSE );
}

/** 启用或禁用Label部件的自动尺寸调整功能 */
LCUI_API void Label_AutoSize(	LCUI_Widget *widget,
				LCUI_BOOL flag,
				AUTOSIZE_MODE mode )
{
	LCUI_Label *label;
	label = (LCUI_Label*)Widget_GetPrivData( widget );
	label->auto_size = flag;
	label->mode = mode;
	Widget_Update( widget );
}
/*-------------------------- End Public ------------------------------*/

/** 注册label部件类型 */
LCUI_API void Register_Label(void)
{
	WidgetType_Add("label");
	WidgetFunc_Add("label",	Label_ExecInit, FUNC_TYPE_INIT);
	WidgetFunc_Add("label",	Label_ExecDraw, FUNC_TYPE_DRAW);
	WidgetFunc_Add("label",	Label_ExecUpdate, FUNC_TYPE_UPDATE);
	WidgetFunc_Add("label", Label_ExecDestroy, FUNC_TYPE_DESTROY);
}
