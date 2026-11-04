#!/usr/bin/env python3
"""Ownership guard for caller-created scratch projects, never editing projects."""

def empty_placeholder(project):
    if not project or project.GetName() != 'Untitled Project' or project.GetTimelineCount() != 0:
        return False
    media = project.GetMediaPool()
    folder = media.GetRootFolder() if media else None
    return bool(folder and folder.GetClipList() == [] and folder.GetSubFolderList() == [])

class ScratchProjectScope:
    def __init__(self, manager, name, allow_empty_placeholder=False):
        self.manager, self.name = manager, name
        self.allow_empty_placeholder = allow_empty_placeholder
        self.project, self.identity = None, None

    def require_empty(self):
        active = self.manager.GetCurrentProject()
        if active and not (self.allow_empty_placeholder and empty_placeholder(active)):
            raise RuntimeError('An editing project is open; refusing to switch projects')

    def bind(self, project):
        if not project:
            raise RuntimeError('Scratch project is unavailable')
        identity = project.GetUniqueId()
        if not isinstance(identity, str) or not identity or project.GetName() != self.name:
            raise RuntimeError('Scratch project ownership could not be verified')
        self.project, self.identity = project, identity
        self.require_current()

    def current(self):
        active = self.manager.GetCurrentProject()
        return bool(self.project and active and active.GetUniqueId() == self.identity
                    and active.GetName() == self.name)

    def require_current(self):
        if not self.current():
            raise RuntimeError('Current project changed; refusing scratch writes or cleanup')

    def save(self):
        self.require_current()
        return bool(self.manager.SaveProject())

    def close(self):
        self.require_current()
        closed = bool(self.manager.CloseProject(self.project))
        if closed:
            self.project, self.identity = None, None
        return closed

    def cleanup(self):
        if not self.project:
            return None
        if not self.current():
            return False
        return self.close()
