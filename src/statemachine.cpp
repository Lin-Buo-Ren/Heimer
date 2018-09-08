// This file is part of Heimer.
// Copyright (C) 2018 Jussi Lind <jussi.lind@iki.fi>
//
// Heimer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// Heimer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Heimer. If not, see <http://www.gnu.org/licenses/>.

#include "statemachine.hpp"
#include "mediator.hpp"

#include "contrib/mclogger.hh"

StateMachine::StateMachine()
{
}

StateMachine::State StateMachine::calculateState(StateMachine::Action action, Mediator & mediator)
{
    switch (action)
    {
    case Action::ExportToPNGSelected:
        m_state = State::ShowExportToPNGDialog;
        break;

    case Action::NewSelected:
        m_quitType = QuitType::New;
        if (mediator.isModified())
        {
            m_state = State::ShowNotSavedDialog;
        }
        else
        {
            m_state = State::InitializeNewMindMap;
        }
        break;

    case Action::MainWindowInitialized:
        m_state = State::InitializeNewMindMap;
        break;

    case Action::NotSavedDialogDiscarded:
        switch (m_quitType)
        {
        case QuitType::Close:
            m_state = State::CloseWindowNow;
            break;
        case QuitType::New:
            m_state = State::InitializeNewMindMap;
            break;
        case QuitType::Open:
            m_state = State::ShowOpenDialog;
            break;
        default:
            m_state = State::Edit;
            break;
        }
        break;

    case Action::ExportedToPNG:
    case Action::NewMindMapInitialized:
    case Action::NotSavedDialogCanceled:
    case Action::MindMapOpened:
    case Action::MindMapSaved:
    case Action::MindMapSavedAs:
    case Action::MindMapSaveFailed:
    case Action::MindMapSaveAsFailed:
        m_quitType = QuitType::None;
        m_state = State::Edit;
        break;

    case Action::OpenSelected:
        m_quitType = QuitType::Open;
        if (mediator.isModified())
        {
            m_state = State::ShowNotSavedDialog;
        }
        else
        {
            m_state = State::ShowOpenDialog;
        }
        break;

    case Action::QuitSelected:
        m_quitType = QuitType::Close;
        if (mediator.isModified())
        {
            m_state = State::ShowNotSavedDialog;
        }
        else
        {
            m_state = State::CloseWindow;
        }
        break;

    case Action::SaveSelected:
        if (mediator.canBeSaved())
        {
            m_state = State::SaveMindMap;
        }
        else
        {
            m_state = State::ShowSaveAsDialog;
        }
        break;

    case Action::SaveAsSelected:
        m_state = State::ShowSaveAsDialog;
        break;

    default:
        MCLogger().warning() << "Action " << static_cast<int>(action) << " not handled!";
    };

    return m_state;
}
