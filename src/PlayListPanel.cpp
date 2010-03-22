// -------------------------------------------------------------------------------- //
//	Copyright (C) 2008-2009 J.Rios
//	anonbeat@gmail.com
//
//    This Program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2, or (at your option)
//    any later version.
//
//    This Program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; see the file LICENSE.  If not, write to
//    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//    http://www.gnu.org/copyleft/gpl.html
//
// -------------------------------------------------------------------------------- //
#include "PlayListPanel.h"

#include "AuiNotebook.h"
#include "AuiDockArt.h"
#include "Commands.h"
#include "Config.h"
#include "DbLibrary.h"
#include "DynamicPlayList.h"
#include "Images.h"
#include "LabelEditor.h"
#include "MainFrame.h"
#include "PlayListAppend.h"
#include "PlayListFile.h"
#include "TagInfo.h"
#include "TrackEdit.h"
#include "Utils.h"

// -------------------------------------------------------------------------------- //
class guPLNamesData : public wxTreeItemData
{
  private :
    int m_Id;
    int m_Type;
  public :
    guPLNamesData( const int id, const int type ) { m_Id = id; m_Type = type; };
    int     GetData( void ) { return m_Id; };
    void    SetData( int id ) { m_Id = id; };
    int     GetType( void ) { return m_Type; };
    void    SetType( int type ) { m_Type = type; };
};


BEGIN_EVENT_TABLE( guPLNamesTreeCtrl, wxTreeCtrl )
    EVT_TREE_BEGIN_DRAG( wxID_ANY, guPLNamesTreeCtrl::OnBeginDrag )
END_EVENT_TABLE()

// -------------------------------------------------------------------------------- //
guPLNamesTreeCtrl::guPLNamesTreeCtrl( wxWindow * parent, guDbLibrary * db ) :
    wxTreeCtrl( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT|wxTR_FULL_ROW_HIGHLIGHT|wxTR_SINGLE|wxSUNKEN_BORDER )
{
    m_Db = db;
    m_ImageList = new wxImageList();
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_track ) ) );
    m_ImageList->Add( wxBitmap( guImage( guIMAGE_INDEX_system_run ) ) );

    AssignImageList( m_ImageList );

    m_RootId   = AddRoot( wxT( "Playlists" ), -1, -1, NULL );
    m_StaticId = AppendItem( m_RootId, _( "Static playlists" ), 0, 0, NULL );
    m_DynamicId = AppendItem( m_RootId, _( "Dynamic playlists" ), 1, 1, NULL );

    SetIndent( 5 );

    SetDropTarget( new guPLNamesDropTarget( this ) );

    Connect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guPLNamesTreeCtrl::OnContextMenu ), NULL, this );

    ReloadItems();
}

// -------------------------------------------------------------------------------- //
guPLNamesTreeCtrl::~guPLNamesTreeCtrl()
{
    Disconnect( wxEVT_COMMAND_TREE_ITEM_MENU, wxTreeEventHandler( guPLNamesTreeCtrl::OnContextMenu ), NULL, this );
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::ReloadItems( void )
{
    int index;
    int count;

    DeleteChildren( m_StaticId );
    DeleteChildren( m_DynamicId );

    guListItems m_StaticItems;
    m_Db->GetPlayLists( &m_StaticItems, GUPLAYLIST_STATIC );
    if( ( count = m_StaticItems.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            AppendItem( m_StaticId, m_StaticItems[ index ].m_Name, 0, 0,
                                new guPLNamesData( m_StaticItems[ index ].m_Id, GUPLAYLIST_STATIC ) );
        }
    }

    guListItems m_DynamicItems;
    m_Db->GetPlayLists( &m_DynamicItems, GUPLAYLIST_DYNAMIC );
    if( ( count = m_DynamicItems.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            AppendItem( m_DynamicId, m_DynamicItems[ index ].m_Name, 1, 1,
                                new guPLNamesData( m_DynamicItems[ index ].m_Id, GUPLAYLIST_DYNAMIC ) );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnContextMenu( wxTreeEvent &event )
{
    wxMenu Menu;
    wxMenuItem * MenuItem;

    wxPoint Point = event.GetPoint();

    wxTreeItemId ItemId = event.GetItem();
    guPLNamesData * ItemData = NULL;

    if( ItemId.IsOk() )
    {
        ItemData = ( guPLNamesData * ) GetItemData( ItemId );

        if( ItemData )
        {
            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_PLAY, _( "Play" ), _( "Play current selected songs" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_player_tiny_light_play ) );
            Menu.Append( MenuItem );

            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_ENQUEUE, _( "Enqueue" ), _( "Add current selected songs to the playlist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_add ) );
            Menu.Append( MenuItem );

            Menu.AppendSeparator();
        }
    }

    MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_NEWPLAYLIST, _( "New Dynamic Playlist" ), _( "Create a new dynamic playlist" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_new ) );
    Menu.Append( MenuItem );

    Menu.AppendSeparator();

    MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_IMPORT, _( "Import" ), _( "Import a playlist" ) );
    MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_new ) );
    Menu.Append( MenuItem );

    if( ItemData )
    {
        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_EXPORT, _( "Export" ), _( "Export the playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_save ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        if( ItemData->GetType() == GUPLAYLIST_DYNAMIC )
        {
            MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_EDIT, _( "Edit Playlist" ), _( "Edit the selected playlist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit ) );
            Menu.Append( MenuItem );

            MenuItem = new wxMenuItem( &Menu, ID_SONG_SAVEPLAYLIST, _( "Save Playlist" ), _( "Save the selected playlist as a Static Playlist" ) );
            MenuItem->SetBitmap( guImage( guIMAGE_INDEX_doc_save ) );
            Menu.Append( MenuItem );
        }

        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_RENAME, _( "Rename Playlist" ), _( "Change the name of the selected playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit ) );
        Menu.Append( MenuItem );

        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_DELETE, _( "Delete Playlist" ), _( "Delete the selected playlist" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit_delete ) );
        Menu.Append( MenuItem );

        Menu.AppendSeparator();

        MenuItem = new wxMenuItem( &Menu, ID_PLAYLIST_COPYTO, _( "Copy to..." ), _( "Copy the current playlist to a directory or device" ) );
        MenuItem->SetBitmap( guImage( guIMAGE_INDEX_edit_copy ) );
        Menu.Append( MenuItem );
    }

    PopupMenu( &Menu, Point );

    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnBeginDrag( wxTreeEvent &event )
{
    wxTreeItemId ItemId = GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( ItemId );
        if( ItemData )
        {
            wxFileDataObject Files;

            if( ItemData->GetType() == GUPLAYLIST_STATIC )
            {
                m_Db->GetPlayListFiles( ItemData->GetData(), &Files );
            }
            else
            {
                guTrackArray Tracks;
                m_Db->GetPlayListSongs( ItemData->GetData(), ItemData->GetType(), &Tracks, NULL, NULL );
                int index;
                int count = Tracks.Count();
                for( index = 0; index < count; index++ )
                {
                    Files.AddFile( Tracks[ index ].m_FileName );
                }
            }

            wxDropSource source( Files, this );

            wxDragResult Result = source.DoDragDrop();
            if( Result )
            {
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDragOver( const wxCoord x, const wxCoord y )
{
    int HitFlags;
    wxTreeItemId TreeItemId = HitTest( wxPoint( x, y ), HitFlags );

    if( TreeItemId.IsOk() )
    {
        if( TreeItemId != m_DragOverItem )
        {
            SetItemDropHighlight( m_DragOverItem, false );
            guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( TreeItemId );
            if( ItemData && ItemData->GetType() == GUPLAYLIST_STATIC )
            {
                SetItemDropHighlight( TreeItemId, true );
                m_DragOverItem = TreeItemId;
            }
        }
    }
    else
    {
        SetItemDropHighlight( m_DragOverItem, false );
        m_DragOverItem = wxTreeItemId();
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDropFile( const wxString &filename )
{
    if( guIsValidAudioFile( filename ) )
    {
        if( wxFileExists( filename ) )
        {
            guTrack Track;
            if( m_Db->FindTrackFile( filename, &Track ) )
            {
                m_DropIds.Add( Track.m_SongId );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesTreeCtrl::OnDropEnd( void )
{
    if( m_DragOverItem.IsOk() )
    {
        SetItemDropHighlight( m_DragOverItem, false );
        if( m_DropIds.Count() )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) GetItemData( m_DragOverItem );
            if( ItemData && ItemData->GetType() == GUPLAYLIST_STATIC )
            {
                m_Db->AppendStaticPlayList( ItemData->GetData(), m_DropIds );
            }

            SelectItem( m_StaticId );
            SelectItem( m_DragOverItem );
        }
        m_DragOverItem = wxTreeItemId();
    }
    m_DropIds.Clear();
}


// -------------------------------------------------------------------------------- //
// guPLNamesDropFilesThread
// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::guPLNamesDropFilesThread( guPLNamesDropTarget * plnamesdroptarget,
                             guPLNamesTreeCtrl * plnamestreectrl, const wxArrayString &files ) :
    wxThread()
{
    m_PLNamesTreeCtrl = plnamestreectrl;
    m_Files = files;
    m_PLNamesDropTarget = plnamesdroptarget;

    if( Create() == wxTHREAD_NO_ERROR )
    {
        SetPriority( WXTHREAD_DEFAULT_PRIORITY );
        Run();
    }
}

// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::~guPLNamesDropFilesThread()
{
//    printf( "guPLNamesDropFilesThread Object destroyed\n" );
    if( !TestDestroy() )
    {
        m_PLNamesDropTarget->ClearPlayListFilesThread();
    }
}

// -------------------------------------------------------------------------------- //
void guPLNamesDropFilesThread::AddDropFiles( const wxString &DirName )
{
    wxDir Dir;
    wxString FileName;
    wxString SavedDir( wxGetCwd() );

    //printf( "Entering Dir : " ); printf( ( char * ) DirName.char_str() );  ; printf( "\n" );
    if( wxDirExists( DirName ) )
    {
        //wxMessageBox( DirName, wxT( "DirName" ) );
        Dir.Open( DirName );
        wxSetWorkingDirectory( DirName );
        if( Dir.IsOpened() )
        {
            if( Dir.GetFirst( &FileName, wxEmptyString, wxDIR_FILES | wxDIR_DIRS ) )
            {
                do {
                    if( ( FileName[ 0 ] != '.' ) )
                    {
                        if( Dir.Exists( FileName ) )
                        {
                            AddDropFiles( DirName + wxT( "/" ) + FileName );
                        }
                        else
                        {
                            m_PLNamesTreeCtrl->OnDropFile( DirName + wxT( "/" ) + FileName );
                        }
                    }
                } while( Dir.GetNext( &FileName ) && !TestDestroy() );
            }
        }
    }
    else
    {
        m_PLNamesTreeCtrl->OnDropFile( DirName );
    }
    wxSetWorkingDirectory( SavedDir );
}

// -------------------------------------------------------------------------------- //
guPLNamesDropFilesThread::ExitCode guPLNamesDropFilesThread::Entry()
{
    int index;
    int Count = m_Files.Count();
    for( index = 0; index < Count; ++index )
    {
        if( TestDestroy() )
            break;
        AddDropFiles( m_Files[ index ] );
    }

    if( !TestDestroy() )
    {
        //
        m_PLNamesTreeCtrl->OnDropEnd();
    }

    return 0;
}

// -------------------------------------------------------------------------------- //
// guPLNamesDropTarget
// -------------------------------------------------------------------------------- //
guPLNamesDropTarget::guPLNamesDropTarget( guPLNamesTreeCtrl * plnamestreectrl )
{
    m_PLNamesTreeCtrl = plnamestreectrl;
    m_PLNamesDropFilesThread = NULL;
}

// -------------------------------------------------------------------------------- //
guPLNamesDropTarget::~guPLNamesDropTarget()
{
//    printf( "guPLNamesDropTarget Object destroyed\n" );
}

// -------------------------------------------------------------------------------- //
bool guPLNamesDropTarget::OnDropFiles( wxCoord x, wxCoord y, const wxArrayString &files )
{
    if( m_PLNamesDropFilesThread )
    {
        m_PLNamesDropFilesThread->Pause();
        m_PLNamesDropFilesThread->Delete();
    }

    m_PLNamesDropFilesThread = new guPLNamesDropFilesThread( this, m_PLNamesTreeCtrl, files );

    if( !m_PLNamesDropFilesThread )
    {
        guLogError( wxT( "Could not create the add files thread." ) );
    }

    return true;
}

// -------------------------------------------------------------------------------- //
wxDragResult guPLNamesDropTarget::OnDragOver( wxCoord x, wxCoord y, wxDragResult def )
{
    //printf( "guPLNamesDropTarget::OnDragOver... %d - %d\n", x, y );
    m_PLNamesTreeCtrl->OnDragOver( x, y );
    return wxDragCopy;
}




// -------------------------------------------------------------------------------- //
// guPlayListPanel
// -------------------------------------------------------------------------------- //
guPlayListPanel::guPlayListPanel( wxWindow * parent, guDbLibrary * db, guPlayerPanel * playerpanel ) :
                 wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL )
{
    m_Db = db;
    m_PlayerPanel = playerpanel;

    guConfig * Config = ( guConfig * ) guConfig::Get();

    m_AuiManager.SetManagedWindow( this );
    m_AuiManager.SetArtProvider( new guAuiDockArt() );
    m_AuiManager.SetFlags( wxAUI_MGR_ALLOW_FLOATING |
                           wxAUI_MGR_TRANSPARENT_DRAG |
                           wxAUI_MGR_TRANSPARENT_HINT );
    wxAuiDockArt * AuiDockArt = m_AuiManager.GetArtProvider();
    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVECAPTIONTEXT ) );
    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVEBORDER ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR,
            wxSystemSettings::GetColour( wxSYS_COLOUR_3DSHADOW ) );

    AuiDockArt->SetColour( wxAUI_DOCKART_GRADIENT_TYPE,
            wxAUI_GRADIENT_VERTICAL );


//	wxBoxSizer* MainSizer;
//	MainSizer = new wxBoxSizer( wxVERTICAL );
//
//	m_MainSplitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D );
//    m_MainSplitter->SetMinimumPaneSize( 60 );

    wxPanel * NamesPanel;
	NamesPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * NameSizer;
	NameSizer = new wxBoxSizer( wxVERTICAL );

//    wxStaticText * m_NameStaticText;
//    m_NameStaticText = new wxStaticText( NamesPanel, wxID_ANY, _( "Play lists:" ) );
//    NameSizer->Add( m_NameStaticText, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5 );
	m_NamesTreeCtrl = new guPLNamesTreeCtrl( NamesPanel, m_Db );
	NameSizer->Add( m_NamesTreeCtrl, 1, wxEXPAND, 5 );

	NamesPanel->SetSizer( NameSizer );
	NamesPanel->Layout();
	NameSizer->Fit( NamesPanel );

    m_AuiManager.AddPane( NamesPanel,
            wxAuiPaneInfo().Name( wxT( "PlayListNames" ) ).Caption( _( "Play Lists" ) ).
            MinSize( 50, 50 ).CloseButton( false ).
            Dockable( true ).Left() );


    wxPanel *  DetailsPanel;
	DetailsPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer * DetailsSizer;
	DetailsSizer = new wxBoxSizer( wxVERTICAL );

	m_PLTracksListBox = new guPLSoListBox( DetailsPanel, m_Db, wxT( "PlayList" ), guLISTVIEW_COLUMN_SELECT );
	DetailsSizer->Add( m_PLTracksListBox, 1, wxEXPAND, 5 );

	DetailsPanel->SetSizer( DetailsSizer );
	DetailsPanel->Layout();
	DetailsSizer->Fit( DetailsPanel );

    m_AuiManager.AddPane( DetailsPanel, wxAuiPaneInfo().Name( wxT( "PlayListTracks" ) ).Caption( wxT( "PlayList" ) ).
            MinSize( 50, 50 ).
            CenterPane() );


    wxString PlayListLayout = Config->ReadStr( wxT( "PlayLists" ), wxEmptyString, wxT( "Positions" ) );
    if( Config->GetIgnoreLayouts() || PlayListLayout.IsEmpty() )
        m_AuiManager.Update();
    else
        m_AuiManager.LoadPerspective( PlayListLayout, true );

//	m_MainSplitter->SplitVertically( NamesPanel, DetailsPanel,
//        Config->ReadNum( wxT( "PlayListSashPos" ), 175, wxT( "Positions" ) ) );
//
//	MainSizer->Add( m_MainSplitter, 1, wxEXPAND, 5 );
//
//	this->SetSizer( MainSizer );
//	this->Layout();

	Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guPlayListPanel::OnPLNamesSelected ), NULL, this );
	Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guPlayListPanel::OnPLNamesActivated ), NULL, this );
    Connect( ID_PLAYLIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesPlay ) );
    Connect( ID_PLAYLIST_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEnqueue ) );
    Connect( ID_PLAYLIST_NEWPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesNewPlaylist ) );
    Connect( ID_PLAYLIST_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEditPlaylist ) );
    Connect( ID_PLAYLIST_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesRenamePlaylist ) );
    Connect( ID_PLAYLIST_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesDeletePlaylist ) );
    Connect( ID_PLAYLIST_COPYTO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesCopyTo ) );

    Connect( ID_PLAYLIST_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesImport ) );
    Connect( ID_PLAYLIST_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesExport ) );

    m_PLTracksListBox->Connect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guPlayListPanel::OnPLTracksActivated ), NULL, this );
    Connect( ID_SONG_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteClicked ) );
    Connect( ID_SONG_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayClicked ) );
    Connect( ID_SONG_PLAYALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayAllClicked ) );
    Connect( ID_SONG_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueClicked ) );
    Connect( ID_SONG_ENQUEUEALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueAllClicked ) );
    Connect( ID_SONG_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditLabelsClicked ) );
    Connect( ID_SONG_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditTracksClicked ) );
    Connect( ID_SONG_COPYTO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksCopyToClicked ) );
    Connect( ID_SONG_SAVEPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSavePlayListClicked ) );

    Connect( ID_SONG_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectGenre ) );
    Connect( ID_SONG_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectArtist ) );
    Connect( ID_SONG_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbum ) );

}

// -------------------------------------------------------------------------------- //
guPlayListPanel::~guPlayListPanel()
{
    // Save the Splitter positions into the main config
    guConfig * Config = ( guConfig * ) guConfig::Get();
    if( Config )
    {
//        Config->WriteNum( wxT( "PlayListSashPos" ), m_MainSplitter->GetSashPosition(), wxT( "Positions" ) );
        Config->WriteStr( wxT( "PlayLists" ), m_AuiManager.SavePerspective(), wxT( "Positions" ) );
    }

	Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( guPlayListPanel::OnPLNamesSelected ), NULL, this );
	Disconnect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( guPlayListPanel::OnPLNamesActivated ), NULL, this );
    Disconnect( ID_PLAYLIST_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesPlay ) );
    Disconnect( ID_PLAYLIST_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEnqueue ) );
    Disconnect( ID_PLAYLIST_NEWPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesNewPlaylist ) );
    Disconnect( ID_PLAYLIST_EDIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesEditPlaylist ) );
    Disconnect( ID_PLAYLIST_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesRenamePlaylist ) );
    Disconnect( ID_PLAYLIST_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesDeletePlaylist ) );
    Disconnect( ID_PLAYLIST_COPYTO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesCopyTo ) );

    Disconnect( ID_PLAYLIST_IMPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesImport ) );
    Disconnect( ID_PLAYLIST_EXPORT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLNamesExport ) );

    m_PLTracksListBox->Disconnect( wxEVT_COMMAND_LISTBOX_DOUBLECLICKED, wxListEventHandler( guPlayListPanel::OnPLTracksActivated ), NULL, this );
    Disconnect( ID_SONG_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksDeleteClicked ) );
    Disconnect( ID_SONG_PLAY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayClicked ) );
    Disconnect( ID_SONG_PLAYALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksPlayAllClicked ) );
    Disconnect( ID_SONG_ENQUEUE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueClicked ) );
    Disconnect( ID_SONG_ENQUEUEALL, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksQueueAllClicked ) );
    Disconnect( ID_SONG_EDITLABELS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditLabelsClicked ) );
    Disconnect( ID_SONG_EDITTRACKS, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksEditTracksClicked ) );
    Disconnect( ID_SONG_COPYTO, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksCopyToClicked ) );
    Disconnect( ID_SONG_SAVEPLAYLIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSavePlayListClicked ) );

    Disconnect( ID_SONG_BROWSE_GENRE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectGenre ) );
    Disconnect( ID_SONG_BROWSE_ARTIST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectArtist ) );
    Disconnect( ID_SONG_BROWSE_ALBUM, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( guPlayListPanel::OnPLTracksSelectAlbum ) );

    m_AuiManager.UnInit();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesSelected( wxTreeEvent& event )
{
    guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( event.GetItem() );
    if( ItemData )
    {
        m_PLTracksListBox->SetPlayList( ItemData->GetData(), ItemData->GetType() );
    }
    else
    {
        m_PLTracksListBox->SetPlayList( -1, -1 );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesActivated( wxTreeEvent& event )
{
    guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( event.GetItem() );
    if( ItemData )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        if( Tracks.Count() )
        {
            guConfig * Config = ( guConfig * ) guConfig::Get();
            if( Config )
            {
                if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "General" ) ) )
                {
                    m_PlayerPanel->AddToPlayList( Tracks );
                }
                else
                {
                    m_PlayerPanel->SetPlayList( Tracks );
                }
            }
        }
    }
    event.Skip();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesPlay( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        m_PlayerPanel->SetPlayList( Tracks );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesEnqueue( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guTrackArray Tracks;
        m_PLTracksListBox->GetAllSongs( &Tracks );
        m_PlayerPanel->AddToPlayList( Tracks );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesNewPlaylist( wxCommandEvent &event )
{
    guDynPlayList DynPlayList;
    guDynPlayListEditor * PlayListEditor = new guDynPlayListEditor( this, &DynPlayList );
    if( PlayListEditor->ShowModal() == wxID_OK )
    {
        PlayListEditor->FillPlayListEditData();

        wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "PlayList Name: " ),
          _( "Enter the new playlist name" ), _( "New Dynamic Playlist" ) );
        if( EntryDialog->ShowModal() == wxID_OK )
        {
            m_Db->CreateDynamicPlayList( EntryDialog->GetValue(), &DynPlayList );
            //m_NamesTreeCtrl->ReloadItems();
            wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
            wxPostEvent( wxTheApp->GetTopWindow(), evt );
        }
        EntryDialog->Destroy();
    }
    PlayListEditor->Destroy();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesEditPlaylist( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
        guDynPlayList DynPlayList;
        m_Db->GetDynamicPlayList( ItemData->GetData(), &DynPlayList );
        guDynPlayListEditor * PlayListEditor = new guDynPlayListEditor( this, &DynPlayList );
        if( PlayListEditor->ShowModal() == wxID_OK )
        {
            PlayListEditor->FillPlayListEditData();
            m_Db->UpdateDynPlayList( ItemData->GetData(), &DynPlayList );
            m_NamesTreeCtrl->ReloadItems();
        }
        PlayListEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesRenamePlaylist( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "PlayList Name: " ),
          _( "Enter the new playlist name" ), m_NamesTreeCtrl->GetItemText( ItemId ) );
        if( EntryDialog->ShowModal() == wxID_OK )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            wxASSERT( ItemData );
            m_Db->SetPlayListName( ItemData->GetData(), EntryDialog->GetValue() );
            //m_NamesTreeCtrl->SetItemText( ItemId, EntryDialog->GetValue() );
            wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
            wxPostEvent( wxTheApp->GetTopWindow(), evt );
        }
        EntryDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesDeletePlaylist( wxCommandEvent &event )
{
    if( wxMessageBox( _( "Are you sure to delete the selected Playlist?" ),
                      _( "Confirm" ),
                      wxICON_QUESTION | wxYES_NO | wxCANCEL, this ) == wxYES )
    {
        wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
        if( ItemId.IsOk() )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            wxASSERT( ItemData );
            m_Db->DeletePlayList( ItemData->GetData() );
            //m_NamesTreeCtrl->Delete( ItemId );
            wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
            wxPostEvent( wxTheApp->GetTopWindow(), evt );
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesCopyTo( wxCommandEvent &event )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guTrackArray * Tracks = new guTrackArray();
        m_PLTracksListBox->GetAllSongs( Tracks );
        event.SetId( ID_MAINFRAME_COPYTO );
        event.SetClientData( ( void * ) Tracks );
        wxPostEvent( wxTheApp->GetTopWindow(), event );
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesImport( wxCommandEvent &event )
{
    int Index;
    int Count;

    wxFileDialog * FileDialog = new wxFileDialog( this,
        wxT( "Select the playlist file" ),
        wxGetHomeDir(),
        wxEmptyString,
        wxT( "*.m3u;*.pls;*.asx;*.xspf" ),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( FileDialog )
    {
        if( FileDialog->ShowModal() == wxID_OK )
        {
            guPlayListFile PlayListFile( FileDialog->GetPath() );
            if( ( Count = PlayListFile.Count() ) )
            {
                if( PlayListFile.GetName().IsEmpty() )
                {
                    wxTextEntryDialog * EntryDialog = new wxTextEntryDialog( this, _( "PlayList Name: " ),
                      _( "Enter the new playlist name" ), _( "New PlayList" ) );
                    if( EntryDialog->ShowModal() == wxID_OK )
                    {
                        PlayListFile.SetName( EntryDialog->GetValue() );
                    }
                    delete EntryDialog;
                }

                //
                if( PlayListFile.GetName().IsEmpty() )
                {
                    PlayListFile.SetName( _( "New PlayList" ) );
                }

                wxArrayInt Songs;
                for( Index = 0; Index < Count; Index++ )
                {
                    wxURI Uri( PlayListFile.GetItem( Index ).m_Location );
                    wxString FileName = Uri.BuildUnescapedURI();
                    if( FileName.StartsWith( wxT( "file:" ) ) )
                        FileName = FileName.Mid( 5 );
                    //guLogMessage( wxT( "Trying to add file '%s'" ), FileName.c_str() );
                    int SongId = m_Db->FindTrackFile( FileName, NULL );
                    if( SongId )
                    {
                        Songs.Add( SongId );
                    //    guLogMessage( wxT( "Found it!" ) );
                    }
                    //else
                    //    guLogMessage( wxT( "Not Found it!" ) );
                }

                if( Songs.Count() )
                {
                    m_Db->CreateStaticPlayList( PlayListFile.GetName(), Songs );
                }

                //m_NamesTreeCtrl->ReloadItems();
                wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
                wxPostEvent( wxTheApp->GetTopWindow(), evt );
            }
        }
        FileDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLNamesExport( wxCommandEvent &event )
{
    int Index;
    int Count;

    wxFileDialog * FileDialog = new wxFileDialog( this,
        wxT( "Select the playlist file" ),
        wxGetHomeDir(),
        wxEmptyString,
        wxT( "*.m3u;*.pls;*.asx;*.xspf" ),
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( FileDialog )
    {
        if( FileDialog->ShowModal() == wxID_OK )
        {
            guPlayListFile PlayListFile;

            wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
            if( ItemId.IsOk() )
            {
                PlayListFile.SetName( m_NamesTreeCtrl->GetItemText( ItemId ) );

                guTrackArray Tracks;

                m_PLTracksListBox->GetAllSongs( &Tracks );
                if( ( Count = Tracks.Count() ) )
                {
                    for( Index = 0; Index < Count; Index++ )
                    {
                        PlayListFile.AddItem( Tracks[ Index ].m_FileName,
                            Tracks[ Index ].m_ArtistName + wxT( " - " ) + Tracks[ Index ].m_SongName );
                    }

                    PlayListFile.Save( FileDialog->GetPath() );
                }
            }
        }
        FileDialog->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksActivated( wxListEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( Tracks.Count() )
    {
        guConfig * Config = ( guConfig * ) guConfig::Get();
        if( Config )
        {
            if( Config->ReadBool( wxT( "DefaultActionEnqueue" ), false, wxT( "General" ) ) )
            {
                m_PlayerPanel->AddToPlayList( Tracks );
            }
            else
            {
                m_PlayerPanel->SetPlayList( Tracks );
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksDeleteClicked( wxCommandEvent &event )
{
    wxArrayInt DelTracks;

    m_PLTracksListBox->GetPlayListSetIds( &DelTracks );

    if( DelTracks.Count() )
    {
        wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
        if( ItemId.IsOk() )
        {
            guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
            if( ItemData && ItemData->GetType() == GUPLAYLIST_STATIC )
            {
                m_Db->DelPlaylistSetIds( ItemData->GetData(), DelTracks );
                m_PLTracksListBox->ReloadItems();
            }
        }
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksPlayClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        m_PLTracksListBox->GetAllSongs( &Tracks );
    m_PlayerPanel->SetPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksPlayAllClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetAllSongs( &Tracks );
    m_PlayerPanel->SetPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksQueueClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        m_PLTracksListBox->GetAllSongs( &Tracks );
    m_PlayerPanel->AddToPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksQueueAllClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetAllSongs( &Tracks );
    m_PlayerPanel->AddToPlayList( Tracks );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksEditLabelsClicked( wxCommandEvent &event )
{
    guListItems Labels;
    wxArrayInt SongIds;

    m_Db->GetLabels( &Labels, true );

    SongIds = m_PLTracksListBox->GetSelectedItems();

    guLabelEditor * LabelEditor = new guLabelEditor( this, m_Db, _( "Songs Labels Editor" ), false,
                         Labels, m_Db->GetSongsLabels( SongIds ) );
    if( LabelEditor )
    {
        if( LabelEditor->ShowModal() == wxID_OK )
        {
            m_Db->UpdateSongsLabels( SongIds, LabelEditor->GetCheckedIds() );
        }
        LabelEditor->Destroy();
        m_PLTracksListBox->ReloadItems();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksEditTracksClicked( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    if( !Tracks.Count() )
        return;
    guImagePtrArray Images;
    wxArrayString Lyrics;

    guTrackEditor * TrackEditor = new guTrackEditor( this, m_Db, &Tracks, &Images, &Lyrics );
    if( TrackEditor )
    {
        if( TrackEditor->ShowModal() == wxID_OK )
        {
            m_Db->UpdateSongs( &Tracks );
            UpdateImages( Tracks, Images );
            UpdateLyrics( Tracks, Lyrics );
            m_PLTracksListBox->ReloadItems();

            // Update the track in database, playlist, etc
            ( ( guMainFrame * ) wxTheApp->GetTopWindow() )->UpdatedTracks( guUPDATED_TRACKS_PLAYLISTS, &Tracks );
        }
        TrackEditor->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksCopyToClicked( wxCommandEvent &event )
{
    guTrackArray * Tracks = new guTrackArray();
    m_PLTracksListBox->GetSelectedSongs( Tracks );

    event.SetId( ID_MAINFRAME_COPYTO );
    event.SetClientData( ( void * ) Tracks );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSavePlayListClicked( wxCommandEvent &event )
{
    int index;
    int count;
    wxArrayInt NewSongs;
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );

    if( ( count = Tracks.Count() ) )
    {
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }
    else
    {
        m_PLTracksListBox->GetAllSongs( &Tracks );
        count = Tracks.Count();
        for( index = 0; index < count; index++ )
        {
            NewSongs.Add( Tracks[ index ].m_SongId );
        }
    }

    if( NewSongs.Count() );
    {
        guListItems PlayLists;
        m_Db->GetPlayLists( &PlayLists,GUPLAYLIST_STATIC );

        guPlayListAppend * PlayListAppendDlg = new guPlayListAppend( wxTheApp->GetTopWindow(), m_Db, &NewSongs, &PlayLists );

        if( PlayListAppendDlg->ShowModal() == wxID_OK )
        {
            int Selected = PlayListAppendDlg->GetSelectedPlayList();
            if( Selected == -1 )
            {
                wxString PLName = PlayListAppendDlg->GetPlaylistName();
                if( PLName.IsEmpty() )
                {
                    PLName = _( "UnNamed" );
                }
                m_Db->CreateStaticPlayList( PLName, NewSongs );
            }
            else
            {
                int PLId = PlayLists[ Selected ].m_Id;
                wxArrayInt OldSongs;
                m_Db->GetPlayListSongIds( PLId, &OldSongs );
                if( PlayListAppendDlg->GetSelectedPosition() == 0 ) // BEGIN
                {
                    m_Db->UpdateStaticPlayList( PLId, NewSongs );
                    m_Db->AppendStaticPlayList( PLId, OldSongs );
                }
                else                                                // END
                {
                    m_Db->AppendStaticPlayList( PLId, NewSongs );
                }
            }
            wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED, ID_PLAYLIST_UPDATED );
            wxPostEvent( wxTheApp->GetTopWindow(), evt );
        }
        PlayListAppendDlg->Destroy();
    }
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::PlayListUpdated( void )
{
    m_NamesTreeCtrl->ReloadItems();
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectGenre( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Genres = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Genres->Index( Tracks[ index ].m_GenreId ) == wxNOT_FOUND )
        {
            Genres->Add( Tracks[ index ].m_GenreId );
        }
    }

    event.SetId( ID_GENRE_SETSELECTION );
    event.SetClientData( ( void * ) Genres );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectArtist( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Artists = new wxArrayInt();
    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Artists->Index( Tracks[ index ].m_ArtistId ) == wxNOT_FOUND )
        {
            Artists->Add( Tracks[ index ].m_ArtistId );
        }
    }
    event.SetId( ID_ARTIST_SETSELECTION );
    event.SetClientData( ( void * ) Artists );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
void guPlayListPanel::OnPLTracksSelectAlbum( wxCommandEvent &event )
{
    guTrackArray Tracks;
    m_PLTracksListBox->GetSelectedSongs( &Tracks );
    wxArrayInt * Albums = new wxArrayInt();

    int index;
    int count = Tracks.Count();
    for( index = 0; index < count; index++ )
    {
        if( Albums->Index( Tracks[ index ].m_AlbumId ) == wxNOT_FOUND )
        {
            Albums->Add( Tracks[ index ].m_AlbumId );
        }
    }
    event.SetId( ID_ALBUM_SETSELECTION );
    event.SetClientData( ( void * ) Albums );
    wxPostEvent( wxTheApp->GetTopWindow(), event );
}

// -------------------------------------------------------------------------------- //
bool guPlayListPanel::GetPlayListCounters( wxLongLong * count, wxLongLong * len, wxLongLong * size )
{
    wxTreeItemId ItemId = m_NamesTreeCtrl->GetSelection();
    if( ItemId.IsOk() )
    {
        guPLNamesData * ItemData = ( guPLNamesData * ) m_NamesTreeCtrl->GetItemData( ItemId );
        if( ItemData )
        {
//            m_Db->GetPlayListCounters( ItemData->GetData(), ItemData->GetType(), count, len, size );
            m_PLTracksListBox->GetCounters( count, len, size );
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------------------------------- //
