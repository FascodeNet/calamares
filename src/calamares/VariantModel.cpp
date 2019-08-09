/* === This file is part of Calamares - <https://github.com/calamares> ===
 *
 *   Copyright 2019, Adriaan de Groot <groot@kde.org>
 *
 *   Calamares is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Calamares is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Calamares. If not, see <http://www.gnu.org/licenses/>.
 */

#include "VariantModel.h"

static void
overallLength( const QVariant& item, int& c, int parent, VariantModel::IndexVector* skiplist )
{
    if ( skiplist )
    {
        skiplist->append( parent );
    }

    parent = c++;
    if ( item.canConvert< QVariantList >() )
    {
        for ( const auto& subitem : item.toList() )
        {
            overallLength( subitem, c, parent, skiplist );
        }
    }
    else if ( item.canConvert< QVariantMap >() )
    {
        for ( const auto& subitem : item.toMap() )
        {
            overallLength( subitem, c, parent, skiplist );
        }
    }
}

static quintptr
findNth( const VariantModel::IndexVector& skiplist, quintptr value, int n )
{
    if ( n < 0 )
    {
        return -1;
    }

    int index = 0;
    while ( ( n >= 0 ) && ( index < skiplist.count() ) )
    {
        if ( skiplist[ index ] == value )
        {
            if ( --n < 0 )
            {
                return index;
            }
        }
        index++;
    }
    return -1;
}


VariantModel::VariantModel( const QVariant* p )
    : m_p( p )
{
    reload();
}

VariantModel::~VariantModel() {}

void
VariantModel::reload()
{
    int x = 0;
    overallLength( *m_p, x, -1, nullptr );
    m_rows.clear();  // Start over
    m_rows.reserve( x );  // We'll need this much
    x = 0;
    overallLength( *m_p, x, -1, &m_rows );
}

int
VariantModel::columnCount( const QModelIndex& index ) const
{
    return 2;
}

int
VariantModel::rowCount( const QModelIndex& index ) const
{
    quintptr p = index.isValid() ? index.internalId() : 0;
    return m_rows.count( p );
}

QModelIndex
VariantModel::index( int row, int column, const QModelIndex& parent ) const
{
    quintptr p = 0;

    if ( parent.isValid() )
    {
        if ( !( parent.internalId() >= m_rows.count() ) )
        {
            p = parent.internalId();
        }
    }

    return createIndex( row, column, findNth( m_rows, p, row ) );
}

QModelIndex
VariantModel::parent( const QModelIndex& index ) const
{
    if ( !index.isValid() || ( index.internalId() > m_rows.count() ) )
    {
        return QModelIndex();
    }

    quintptr p = m_rows[ index.internalId() ];
    if ( p == 0 )
    {
        return QModelIndex();
    }

    if ( p >= m_rows.count() )
    {
        return QModelIndex();
    }
    quintptr p_pid = m_rows[ p ];
    int row = 0;
    for ( int i = 0; i < p; ++i )
    {
        if ( m_rows[ i ] == p_pid )
        {
            row++;
        }
    }

    return createIndex( row, index.column(), p );
}

QVariant
VariantModel::data( const QModelIndex& index, int role ) const
{
    if ( role != Qt::DisplayRole )
    {
        return QVariant();
    }

    if ( !index.isValid() )
    {
        return QVariant();
    }

    if ( ( index.column() < 0 ) || ( index.column() > 1 ) )
    {
        return QVariant();
    }

    if ( index.internalId() >= m_rows.count() )
    {
        return QVariant();
    }

    const QVariant thing = underlying( parent( index ) );

    if ( !thing.isValid() )
    {
        return QVariant();
    }

    if ( thing.canConvert< QVariantMap >() )
    {
        QVariantMap the_map = thing.toMap();
        const auto key = the_map.keys().at( index.row() );
        if ( index.column() == 0 )
        {
            return key;
        }
        else
        {
            return the_map[ key ];
        }
    }
    else if ( thing.canConvert< QVariantList >() )
    {
        if ( index.column() == 0 )
        {
            return index.row();
        }
        else
        {
            QVariantList the_list = thing.toList();
            return the_list.at( index.row() );
        }
    }
    else
    {
        if ( index.column() == 0 )
        {
            return QVariant();
        }
        else
        {
            return thing;
        }
    }
}

QVariant
VariantModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if ( role != Qt::DisplayRole )
    {
        return QVariant();
    }

    if ( orientation == Qt::Horizontal )
    {
        if ( section == 0 )
        {
            return tr( "Key" );
        }
        else if ( section == 1 )
        {
            return tr( "Value" );
        }
        else
        {
            return QVariant();
        }
    }
    else
    {
        return QVariant();
    }
}

const QVariant
VariantModel::underlying( const QModelIndex& index ) const
{
    if ( !index.isValid() )
    {
        return *m_p;
    }

    const auto& thing = underlying( parent( index ) );
    if ( thing.canConvert< QVariantMap >() )
    {
        const auto& the_map = thing.toMap();
        return the_map[ the_map.keys()[ index.row() ] ];
    }
    else if ( thing.canConvert< QVariantList >() )
    {
        return thing.toList()[ index.row() ];
    }
    else
    {
        return thing;
    }

    return QVariant();
}